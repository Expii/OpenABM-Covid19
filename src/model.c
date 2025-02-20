 /* model.c
 *
 *  Created on: 5 Mar 2020
 *      Author: hinchr
 */

#include "model.h"
#include "individual.h"
#include "utilities.h"
#include "constant.h"
#include "params.h"
#include "network.h"
#include "disease.h"
#include "interventions.h"
#include "demographics.h"
#include "hospital.h"
#include "hashset.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/*****************************************************************************************
*  Name:		new_model
*  Description: Builds a new model object from a parameters object and returns a
*  				pointer to it.
*  				 1. Creates memory for it
*  				 2. Initialises the gsl random numbers generator
*  Returns:		pointer to model
******************************************************************************************/
model* new_model( parameters *params )
{
	int type;

	model *model_ptr = NULL;
	model_ptr = calloc( 1, sizeof( model ) );
	if( model_ptr == NULL )
	    print_exit("calloc to model failed\n");
	
	model_ptr->params = params;
	model_ptr->time   = 0;
	model_ptr->rebuild_networks = TRUE;
	model_ptr->user_network = NULL;
	model_ptr->n_initialised_strains = 0;
    if (params->occupation_network_table == NULL)
    {
        model_ptr->use_custom_occupation_networks = 0;
        model_ptr->n_occupation_networks = N_DEFAULT_OCCUPATION_NETWORKS;
        set_up_default_occupation_network_table( params );
    }
    else
    {
        model_ptr->use_custom_occupation_networks = 1;
        model_ptr->n_occupation_networks = params->occupation_network_table->n_networks;
    }

    gsl_rng_env_setup();
    rng = gsl_rng_alloc ( gsl_rng_default);
    gsl_rng_set( rng, params->rng_seed );

	update_intervention_policy( model_ptr, model_ptr->time );

	model_ptr->event_lists = calloc( N_EVENT_TYPES, sizeof( event_list ) );
	for( type = 0; type < N_EVENT_TYPES;  type++ )
		set_up_event_list( model_ptr, params, type );

	set_up_counters( model_ptr );
	set_up_population( model_ptr );
	set_up_household_distribution( model_ptr );
	set_up_allocate_work_places( model_ptr );
	if( params->hospital_on )
		set_up_healthcare_workers_and_hospitals( model_ptr );
	set_up_networks( model_ptr );
	set_up_interactions( model_ptr );
	set_up_events( model_ptr );
	set_up_transition_times( model_ptr );
	set_up_transition_times_intervention( model_ptr );
	set_up_infectious_curves( model_ptr );
	set_up_individual_hazard( model_ptr );
	set_up_strains( model_ptr );
	set_up_seed_infection( model_ptr );
	set_up_app_users( model_ptr );
	set_up_trace_tokens( model_ptr, 0.01 );
	set_up_risk_scores( model_ptr );
	if (params->novid_on)
		set_up_novid_network( model_ptr );

	model_ptr->n_quarantine_days = 0;

	return model_ptr;
}

/*****************************************************************************************
*  Name:		destroy_model
*  Description: Destroys the model structure and releases its memory
******************************************************************************************/
void destroy_model( model *model )
{
	long idx;
	int ddx;
	network *network, *next_network;
	interaction_block *interaction_block, *next_interaction_block;
	trace_token_block *trace_token_block, *next_trace_token_block;
	event_block *event_block, *next_event_block;

	for( idx = 0; idx < model->params->n_total; idx++ )
		destroy_individual( &(model->population[idx] ) );
	free( model->population );
	free( model->possible_interactions );

	for( ddx = 0; ddx < model->params->days_of_interactions; ddx++ )
	{
		next_interaction_block = model->interaction_blocks[ddx];
		while( next_interaction_block != NULL )
		{
			interaction_block = next_interaction_block;
			next_interaction_block = interaction_block->next;
			if( interaction_block->interactions != NULL )
				free( interaction_block->interactions );
			free( interaction_block );
		}
	}

    next_event_block = model->event_block;
	while( next_event_block != NULL )
	{
		event_block = next_event_block;
		next_event_block = event_block->next;
		if( event_block->events != NULL )
			free( event_block->events );
		free( event_block );
	}


	for( idx = 0; idx < N_TRANSITION_TYPES; idx++ )
		free( model->transition_time_distributions[ idx ] );
	free( model->transition_time_distributions );

    destroy_network( model->random_network);
    destroy_network( model->household_network );
    for( idx = 0; idx < model->n_occupation_networks; idx++ )
    	destroy_network( model->occupation_network[idx] );
    free( model->occupation_network );

    next_network = model->user_network;
	while( next_network != NULL )
	{
		network = next_network;
		next_network = network->next_network;
		destroy_network(network);
	}

	for( idx = 0; idx < N_EVENT_TYPES; idx++ )
		destroy_event_list( model, idx );

    free( model->event_lists );
    for( idx = 0; idx < model->household_directory->n_idx; idx++ )
    	free( model->household_directory->val[idx] );
    free( model->household_directory->val );
    free( model->household_directory->n_jdx );
    free ( model-> household_directory );

    next_trace_token_block = model->trace_token_block;
	while( next_trace_token_block != NULL )
	{
		trace_token_block = next_trace_token_block;
		next_trace_token_block = trace_token_block->next;
		if( trace_token_block->trace_tokens != NULL )
			free( trace_token_block->trace_tokens );
		free( trace_token_block );
	}

    if( model->params->hospital_on )
    {
    	for( idx = 0; idx < model->params->n_hospitals; idx++)
    		destroy_hospital( &(model->hospitals[idx]) );
    	free( model->hospitals );
    }
    destroy_risk_scores( model );

    free( model->strains );
    for( idx = 0; idx < model->params->max_n_strains; idx++ )
		free( model->cross_immunity[idx] );
	free( model->cross_immunity );

    free( model );
}

/*****************************************************************************************
*  Name:		set_up_event_list
*  Description: sets up an event_list
*  Returns:		voidx
******************************************************************************************/
void set_up_event_list( model *model, parameters *params, int type )
{

	int day, age, idx;
	event_list *list = &(model->event_lists[ type ]);
	list->type       = type;

	list->n_daily          = calloc( MAX_TIME, sizeof(long) );
	list->n_daily_current  = calloc( MAX_TIME, sizeof(long) );
	list->infectious_curve = calloc( N_INTERACTION_TYPES, sizeof(double*) );
	list->n_total_by_age   = calloc( N_AGE_GROUPS, sizeof(long) );
	list->n_daily_by_age   = calloc( MAX_TIME, sizeof(long*) );
	list->events		   = calloc( MAX_TIME, sizeof(event*));

	list->n_current = 0;
	list->n_total   = 0;
	for( day = 0; day < MAX_TIME; day++ )
	{
		list->n_daily_by_age[day] = calloc( N_AGE_GROUPS, sizeof(long) );
		for( age = 0; age < N_AGE_GROUPS; age++ )
			list->n_daily_by_age[day][age] = 0;

		list->n_daily[day] = 0;
		list->n_daily_current[day] = 0;
	}
	for( idx = 0; idx < N_INTERACTION_TYPES; idx++ )
		list->infectious_curve[idx] = calloc( MAX_INFECTIOUS_PERIOD, sizeof(double) );
}

/*****************************************************************************************
*  Name:		destroy_event_list
*  Description: Destroys an event list
******************************************************************************************/
void destroy_event_list( model *model, int type )
{
	int day, idx;
	free( model->event_lists[type].n_daily );

	for( day = 0; day < MAX_TIME; day++ )
		free( model->event_lists[type].n_daily_by_age[day]);
	for( idx = 0; idx < N_INTERACTION_TYPES; idx++ )
		free( model->event_lists[type].infectious_curve[idx] );

	free( model->event_lists[type].n_daily_current );
	free( model->event_lists[type].infectious_curve );
	free( model->event_lists[type].n_total_by_age );
	free( model->event_lists[type].n_daily_by_age );
	free( model->event_lists[type].events );
}

/*****************************************************************************************
*  Name:		set_up_networks
*  Description: sets up then networks
*  Returns:		void
******************************************************************************************/
void set_up_networks( model *model )
{
	long idx;
	long n_total 			  = model->params->n_total;
	long n_random_interactions;
	double mean_interactions  = 0;

	for( idx = 0; idx < N_AGE_TYPES; idx++ )
		mean_interactions = max( mean_interactions, model->params->mean_random_interactions[idx] );
	n_random_interactions = (long) round( n_total * ( 1.0 + mean_interactions ) );

	model->random_network        = create_network( n_total, RANDOM );
	model->random_network->edges = calloc( n_random_interactions, sizeof( edge ) );
	model->random_network->skip_hospitalised = FALSE;
	model->random_network->skip_quarantined  = FALSE;
	model->random_network->construction      = NETWORK_CONSTRUCTION_RANDOM_DEFAULT;
	model->random_network->daily_fraction    = 1.0;
	model->random_network->network_id        = RANDOM_NETWORK;
	strcpy( model->random_network->name, DEFAULT_NETWORKS_NAMES[RANDOM_NETWORK] );

	model->household_network = create_network( n_total, HOUSEHOLD );
	build_household_network_from_directroy( model->household_network, model->household_directory );
	model->household_network->skip_hospitalised = TRUE;
	model->household_network->skip_quarantined  = FALSE;
	model->household_network->construction      = NETWORK_CONSTRUCTION_HOUSEHOLD;
	model->household_network->daily_fraction    = 1.0;
	model->household_network->network_id        = HOUSEHOLD_NETWORK;
	strcpy( model->household_network->name, DEFAULT_NETWORKS_NAMES[HOUSEHOLD_NETWORK] );

    set_up_occupation_network( model );

	if( model->params->hospital_on )
		set_up_hospital_networks( model );

	model->mean_interactions = estimate_mean_interactions_by_age( model, -1 );
	for( idx = 0; idx < N_AGE_TYPES; idx++ )
		model->mean_interactions_by_age[idx] = estimate_mean_interactions_by_age( model, idx );
}

/*****************************************************************************************
*  Name:		set_up_novid_network
*  Description: sets up NOVID network
*  Returns:		void
******************************************************************************************/
void set_up_novid_network( model *model )
{
	long n_total = model->params->n_total;
	individual *indivs = model->population;

	hashset **all = calloc(n_total, sizeof(hashset*));
	hashset ***adj_set = calloc(n_total, sizeof(hashset**));
	for (long i = 0; i < n_total; i++) {
		all[i] = init_set();
		set_insert(all[i], i);
		adj_set[i] = calloc(MAX_NOVID_DIST, sizeof(hashset*));
		for (int j = 0; j < MAX_NOVID_DIST; j++)
			adj_set[i][j] = init_set();
	}

	network *net;
	for (int j = 0; j <= model->n_occupation_networks; j++) {
		if (j < model->n_occupation_networks)
			net = model->occupation_network[j];
		else
			net = model->household_network;

		for (long i = 0; i < net->n_edges; i++) {
			long a = net->edges[i].id1;
			long b = net->edges[i].id2;
			if (indivs[a].app_user && indivs[b].app_user) {
				set_insert(adj_set[a][1], b);
				set_insert(adj_set[b][1], a);
				set_insert(all[a], b);
				set_insert(all[b], a);
			}
		}
	}

	for (long i = 0; i < n_total; i++) {
		indivs[i].novid_n_adj[0] = indivs[i].app_user;
		indivs[i].novid_adj_list[0] = calloc(indivs[i].app_user, sizeof(long));
		indivs[i].novid_adj_list[0][0] = i;
		indivs[i].novid_n_adj[1] = set_size(adj_set[i][1]);
		indivs[i].novid_adj_list[1] = set_to_list(adj_set[i][1]);
	}
	for (int d = 2; d < MAX_NOVID_DIST; d++) {
		for (long i = 0; i < n_total; i++) {
			for (long j = 0; j < indivs[i].novid_n_adj[d-1]; j++) {
				long v = indivs[i].novid_adj_list[d-1][j];
				for (int k = 0; k < indivs[v].novid_n_adj[1]; k++) {
					long w = indivs[v].novid_adj_list[1][k];
					if (!set_contains(all[i], w)) {
						set_insert(adj_set[i][d], w);
						set_insert(all[i], w);
					}
				}
			}
			indivs[i].novid_n_adj[d] = set_size(adj_set[i][d]);
			indivs[i].novid_adj_list[d] = set_to_list(adj_set[i][d]);
		}
	}

	long total[MAX_NOVID_DIST], freq[30];
	for (int d = 0; d < MAX_NOVID_DIST; d++) {
		total[d] = 0;
		for (long i = 0; i < n_total; i++) {
			total[d] += indivs[i].novid_n_adj[d];
		}
		if (DEBUG) printf("avg_degree[%d] = %lf\n", d, 1.0*total[d]/n_total);
	}
	for (int i = 0; i<30; i++)
		freq[i] = 0;
	for (long i = 0; i < n_total; i++)
		freq[min(indivs[i].novid_n_adj[1],29)]++;
	if (DEBUG) {
		for (int i = 0; i<30; i++)
			printf("deg %d: %ld\n", i, freq[i]);
	}

	for (long i = 0; i < n_total; i++) {
		destroy_set(all[i]);
		for (int j = 0; j < MAX_NOVID_DIST; j++) {
			destroy_set(adj_set[i][j]);
		}
		free(adj_set[i]);
	}
	free(all);
	free(adj_set);
}

/*****************************************************************************************
*  Name:		set_up_counters
*  Description: sets up counters of events
*  Returns:		void
******************************************************************************************/
void set_up_counters( model *model ){
	
	int idx;

	model->n_quarantine_infected = 0;
	model->n_quarantine_recovered = 0;
	model->n_quarantine_app_user = 0;
	model->n_quarantine_app_user_infected = 0;
	model->n_quarantine_app_user_recovered = 0;
	model->n_app_user_infected = 0;
	// Daily totals
	model->n_quarantine_events = 0;
	model->n_quarantine_events_app_user = 0;
	model->n_quarantine_release_events = 0;
	model->n_quarantine_release_events_app_user = 0;

	model->n_vaccinated_fully = 0;
	model->n_vaccinated_symptoms = 0;
	for( idx = 0; idx < N_AGE_GROUPS; idx++ )
	{
		model->n_population_by_age[ idx ] = 0;
		model->n_vaccinated_fully_by_age[ idx ] = 0;
		model->n_vaccinated_symptoms_by_age[ idx ] = 0;
	}
}

/*****************************************************************************************
*  Name:		reset_counters
*  Description: reset counters of events
*  Returns:		void
******************************************************************************************/
void reset_counters( model *model ){

	model->n_quarantine_events = 0;
	model->n_quarantine_events_app_user = 0;
	model->n_quarantine_release_events = 0;
	model->n_quarantine_release_events_app_user = 0;
}

/*****************************************************************************************
*  Name:		set_up_occupation_network
*  Description: sets up the work network
*  Returns:		void
******************************************************************************************/
void set_up_occupation_network( model *model )
{
	long idx, n_people ;
	long *people;
	double n_interactions;
	parameters *params = model->params;

    model->occupation_network = calloc( model->n_occupation_networks, sizeof( network* ) );
    for( int network = 0; network < model->n_occupation_networks; network++ )
    {
        n_people = 0;
        people = calloc( params->n_total, sizeof(long) );
		for ( idx = 0; idx < params->n_total; idx++ )
			if (model->population[idx].occupation_network == network)
				people[n_people++] = idx;

        model->occupation_network[network] = create_network( n_people, OCCUPATION );
        model->occupation_network[network]->skip_hospitalised = TRUE;
        model->occupation_network[network]->skip_quarantined  = TRUE;
        model->occupation_network[network]->construction      = NETWORK_CONSTRUCTION_WATTS_STROGATZ;
        model->occupation_network[network]->daily_fraction = model->params->daily_fraction_work;
        model->occupation_network[network]->network_id = params->occupation_network_table->network_ids[network];
        model->occupation_network[network]->n_edges = 0;
        strcpy( model->occupation_network[network]->name, params->occupation_network_table->network_names[network] );
        n_interactions = params->occupation_network_table->mean_interactions[network] / params->daily_fraction_work;

        if ( n_people > 0 )
        {
            build_watts_strogatz_network( model->occupation_network[network], n_people, n_interactions,
                                         params->work_network_rewire, TRUE );
            relabel_network( model->occupation_network[network], people );
        }

        free( people );
    }

    if( model->use_custom_occupation_networks == FALSE )
    {
    	free( params->occupation_network_table );
    	params->occupation_network_table = NULL;
    }
}

/*****************************************************************************************
*  Name:		set_up_events
*  Description: sets up the event tags
*  Returns:		void
******************************************************************************************/
void set_up_events( model *model )
{
	model->event_block = NULL;
	model->next_event  = NULL;
	add_event_block( model, 1.0 );
}

/*****************************************************************************************
*  Name:		add_event_block
*  Description: sets up the event tags
*  Returns:		void
******************************************************************************************/
void add_event_block( model *model, float events_per_person )
{
	long idx;
	long n_events = ceil( model->params->n_total * events_per_person );
	event_block *block;

	// add a new block
	block = calloc( 1, sizeof( event_block ) );
	block->next = model->event_block;
	model->event_block = block;

	block->events     = calloc( n_events, sizeof( event ) );
	model->next_event = &(block->events[0]);
	for( idx = 1; idx < n_events; idx++ )
	{
		block->events[idx-1].next = &(block->events[idx]);
		block->events[idx].last   = &(block->events[idx-1]);
	}
	block->events[ n_events - 1].next = NULL;
}


/*****************************************************************************************
*  Name:		set_up_population
*  Description: sets up the initial population
*  Returns:		void
******************************************************************************************/
void set_up_population( model *model )
{
	parameters *params = model->params;
	long idx;

	model->population = calloc( params->n_total, sizeof( individual ) );
	for( idx = 0; idx < params->n_total; idx++ )
		initialize_individual( &(model->population[idx]), params, idx );
}

/*****************************************************************************************
*  Name:		set_up_individual_hazard
*  Description: sets the initial hazard for each individual
*  Returns:		void
******************************************************************************************/
void set_up_individual_hazard( model *model )
{
	parameters *params = model->params;
	long idx;

	for( idx = 0; idx < params->n_total; idx++ )
		initialize_hazard( &(model->population[idx]), params, 0 );
}

/*****************************************************************************************
*  Name:		estimate_total_interactions
*  Description: estimates the total number of interactions from the networks
*  Returns:		void
******************************************************************************************/
double estimate_total_interactions( model *model )
{
	long idx;
	double n_interactions;
	n_interactions = 0;

	n_interactions += model->household_network->n_edges;
	for( idx = 0; idx < model->params->n_total; idx++ )
		n_interactions += model->population[idx].base_random_interactions * 0.5;
	for( idx = 0; idx < model->n_occupation_networks ; idx++ )
		n_interactions += model->occupation_network[idx]->n_edges * model->params->daily_fraction_work;

	if( model->params->hospital_on )
        for( int hospital_idx = 0; hospital_idx < model->params->n_hospitals; hospital_idx++)
			n_interactions += model->hospitals[hospital_idx].hospital_workplace_network->n_edges;

	return n_interactions;
}

/*****************************************************************************************
*  Name:		add_interaction_block
*  Description: adds a block of interactions (required to be dynamic if size of network
*  				can change
*  Returns:		void
******************************************************************************************/
void add_interaction_block( model *model, long n_interactions )
{
	long ddx;
	interaction_block *block;

	for( ddx = 0; ddx < model->params->days_of_interactions; ddx++ )
	{
		block = calloc( 1, sizeof( interaction_block ) );
		block->interactions = calloc( n_interactions, sizeof( interaction ) );
		block->n_interactions = n_interactions;
		block->idx = 0;

		if( model->interaction_blocks[ddx] != NULL )
			block->next = model->interaction_blocks[ddx];

		model->interaction_blocks[ddx] = block;
	}
}

/*****************************************************************************************
*  Name:		set_up_interactions
*  Description: sets up the stock of interactions, note that these get recycled once we
*  				move to a later date
*  Returns:		void
******************************************************************************************/
void set_up_interactions( model *model )
{
	parameters *params = model->params;
	individual *indiv;
	long idx, n_idx, indiv_idx, n_daily_interactions, n_random_interactions;

	n_daily_interactions = (long) round( 2 * 1.1 * estimate_total_interactions( model ) );

	model->interaction_blocks = calloc( params->days_of_interactions, sizeof( interaction_block* ) );
	add_interaction_block( model, n_daily_interactions );
	model->interaction_day_idx   = 0;

	// count the number of random interactions
	n_random_interactions = 0;
	for( indiv_idx = 0; indiv_idx < params->n_total; indiv_idx++ )
		n_random_interactions += model->population[ indiv_idx ].random_interactions;

	model->possible_interactions = calloc( ceil( n_random_interactions * 1.1 ), sizeof( long ) );
	idx = 0;
	for( indiv_idx = 0; indiv_idx < params->n_total; indiv_idx++ )
	{
		indiv = &(model->population[ indiv_idx ]);
		for( n_idx = 0; n_idx < indiv->random_interactions; n_idx++ )
			model->possible_interactions[ idx++ ] = indiv_idx;
	}

	model->n_possible_interactions = idx;
	model->n_total_intereactions   = 0;
}


/*****************************************************************************************
*  Name:		new_event
*  Description: gets a new event tag
*  Returns:		void
******************************************************************************************/
event* new_event( model *model )
{
	event *event = model->next_event;

	if( event->next == NULL )
	{
		add_event_block( model, 0.5 );
	}
	else
	{
		model->next_event       = event->next;
		model->next_event->last = NULL;
	}

	event->next = NULL;
	event->last = NULL;

	return event;
}


/*****************************************************************************************
*  Name:		flu_infections
*  Description: Randomly pick people from the population to go down with flu, they
*  				will then self-quarantine (same fraction of true infected people)
*  				and request a test.
*
*  Returns:		void
******************************************************************************************/
void flu_infections( model *model )
{
	long idx, pdx, n_infected;
	individual *indiv;

	n_infected = round( model->params->n_total * model->params->daily_non_cov_symptoms_rate );

	for( idx = 0; idx < n_infected; idx++ )
	{
		pdx   = gsl_rng_uniform_int( rng, model->params->n_total );
		indiv = &(model->population[pdx]);

		if( is_in_hospital( indiv ) || indiv->status == DEATH )
			continue;

		intervention_on_symptoms( model, indiv );
	}

}

/*****************************************************************************************
*  Name:		add_indiv_to_event_list
*  Description: adds an individual to an event list at a particular time
*
*  Arguments:	list:	pointer to the event list
*  				indiv:	pointer to the individual
*  				time:	time of the event (int)
*  				model:	pointer to the model
*  				info:   a pointer which can be passed to the transition function
*
*  Returns:		a pointer to the newly added event
******************************************************************************************/
event* add_individual_to_event_list(
	model *model,
	int type,
	individual *indiv,
	int time,
	void *info
)
{
	event_list *list    = &(model->event_lists[ type ]);
	event *event        = new_event( model );
	event->individual   = indiv;
	event->type         = type;
	event->time         = time;
	event->info  = info;

	if( time < MAX_TIME){
		if( list->n_daily_current[time] >0  )
		{
			list->events[ time ]->last = event;
			event->next  = list->events[ time ];
		}

		list->events[time ] = event;
		list->n_daily[time]++;
		list->n_daily_by_age[time][indiv->age_group]++;
		list->n_daily_current[time]++;
	}

	if( time <= model->time )
	{
		list->n_total++;
		list->n_current++;
		list->n_total_by_age[indiv->age_group]++;
	}

	return event;
}

/*****************************************************************************************
*  Name:		remove_event_from_event_list
*  Description: removes an event from an list at a particular time
*
*  Arguments:	list:	pointer to the event list
*  				event:	pointer to the event
*  				time:	time of the event (int)
*
*  Returns:		a pointer to the newly added event
******************************************************************************************/
void remove_event_from_event_list(
	model *model,
	event *event
)
{
	int type = event->type;
	int time = event->time;
	event_list *list = &(model->event_lists[ type ]);

	if( list->n_daily_current[ time ] > 1 )
	{
		if( event != list->events[ time ] )
		{
			if( event->next == NULL )
				event->last->next = NULL;
			else
			{
				event->last->next = event->next;
				event->next->last = event->last;
			}
		}
		else
		{
			list->events[ time ] = event->next;
			list->events[ time ]->last = NULL;
		}
	}
	else
		list->events[time] = NULL;

	// return to the stack
	model->next_event->last = event;
	event->next = model->next_event;
	model->next_event = event;

	if( time <= model->time )
		list->n_current--;
	list->n_daily_current[ time ]--;
}

/*****************************************************************************************
*  Name:		update_event_list_counters
*  Description: updates the event list counters, called at the end of a time step
*  Returns:		void
******************************************************************************************/
void update_event_list_counters( model *model, int type )
{
	model->event_lists[type].n_current += model->event_lists[type].n_daily_current[ model->time ];
	model->event_lists[type].n_total   += model->event_lists[type].n_daily[ model->time ];

	for( int age = 0; age < N_AGE_GROUPS; age++ )
		model->event_lists[type].n_total_by_age[age] += model->event_lists[type].n_daily_by_age[ model->time ][ age ];
}

/*****************************************************************************************
*  Name:		set_up_strains
*  Description: allocates memory for strains and cross-immunity matrix
*  Returns:		void
******************************************************************************************/
void set_up_strains( model *model )
{
	int max_n_strains = model->params->max_n_strains;
	model->strains = calloc( max_n_strains, sizeof( strain ) );

	float** cross_immunity;
	int jdx;
	cross_immunity = calloc( max_n_strains, sizeof(float *) );
	for( int idx = 0; idx < max_n_strains; idx++)
	{
		cross_immunity[idx] 		= calloc( max_n_strains, sizeof(float) );
		for( jdx = 0; jdx < max_n_strains; jdx++)
			cross_immunity[idx][jdx] 	= 1; // set complete cross-immunity
		model->strains[idx].idx 	= -1; // if idx = -1, strain is uninitialised
	}
	model->cross_immunity = cross_immunity;
}

/*****************************************************************************************
*  Name:		set_up_seed_infection
*  Description: sets up the initial population
*  Returns:		void
******************************************************************************************/
void set_up_seed_infection( model *model )
{
	parameters *params = model->params;
	int idx, strain_idx;
	unsigned long int person;
	individual *indiv;
	double *hospitalised_fraction = calloc( N_AGE_GROUPS, sizeof( double  ) );

	for( idx = 0; idx < N_AGE_GROUPS; idx++ )
		hospitalised_fraction[ idx ] = params->hospitalised_fraction[ idx ];

	idx = 0;
	strain_idx = add_new_strain( model, 1, hospitalised_fraction );

	while( idx < params->n_seed_infection )
	{
		person = gsl_rng_uniform_int( rng, params->n_total );
		indiv  = &(model->population[ person ]);

		if( time_infected( indiv ) != NO_EVENT )
			continue;

		if( !params->hospital_on || indiv->worker_type == NOT_HEALTHCARE_WORKER )
		{
			if( seed_infect_by_idx( model, indiv->idx, strain_idx, -1 ) )
				idx++;
		}
	}

	free( hospitalised_fraction );
}

/*****************************************************************************************
*  Name:		new_seed_infections
*  Description: add additional infections each day
*  Returns:		void
******************************************************************************************/
void new_seed_infections( model *model )
{
	parameters *params = model->params;
	int strain_idx = 0;
	int end = round( params->new_seed_infection_rate * model->time );
	int start = round( params->new_seed_infection_rate * (model->time - 1) );
	int idx = 0;
	int num_infect = end-start;
	unsigned long int person;
	individual *indiv;

	while( idx < num_infect )
	{
		person = gsl_rng_uniform_int( rng, params->n_total );
		indiv  = &(model->population[ person ]);

		if( time_infected( indiv ) != NO_EVENT )
			continue;

		if( !params->hospital_on || indiv->worker_type == NOT_HEALTHCARE_WORKER )
		{
			if( seed_infect_by_idx( model, indiv->idx, strain_idx, -1 ) )
				idx++;
		}
	}
}

/*****************************************************************************************
*  Name:		build_random_network
*  Description: Builds a new random network
******************************************************************************************/
void build_random_network( model *model, network *network, long n_pos, long* interactions )
{
	long idx, temp, skip;
	if( ( n_pos == 0 ) || ( network->daily_fraction < 1e-9 ) )
		return;

	gsl_ran_shuffle( rng, interactions, n_pos, sizeof(long) );

	network->n_edges = 0;
	idx  = 0;
	n_pos--;
	while( idx < n_pos )
	{
		if( interactions[ idx ] == interactions[ idx + 1 ] )
		{
			skip = 1;
			while( ( idx + skip ) < n_pos )
			{
				if( interactions[ idx ] != interactions[ idx + 1 + skip ] )
				{
					temp = interactions[ idx + 1 + skip ];
					interactions[ idx + 1 + skip ] = interactions[ idx + 1 ];
					interactions[ idx + 1 ] = temp;
					break;
				}
				skip++;
			};

			if( interactions[ idx ] == interactions[ idx + 1  ] )
			{
				idx++;
				continue;
			}
		}
		network->edges[network->n_edges].id1 = interactions[ idx++ ];
		network->edges[network->n_edges].id2 = interactions[ idx++ ];
		network->n_edges++;
		skip = 1;
	}
}

/*****************************************************************************************
*  Name:		build_random_network_user
*  Description: Builds a new random user defined network
******************************************************************************************/
void build_random_network_user( model *model, network *network )
{
	if( network->daily_fraction < 1e-9 )
		return;

	long idx, pdx, n_pos;
	int jdx;
	individual *indiv;

	n_pos  = 0;
	for( idx = 0; idx < network->opt_n_indiv; idx++ )
	{
		pdx   = network->opt_pdx_array[ idx ];
		indiv = &(model->population[ pdx ] );

		if( network->skip_hospitalised && is_in_hospital( indiv ) )
			continue;
		if( network->skip_quarantined && indiv->quarantined && !model->params->soft_quarantine_on )
			continue;

		for( jdx = 0; jdx < network->opt_int_array[ idx ]; jdx++ )
			network->opt_long_array[ n_pos++ ] = pdx;
	}

	build_random_network( model, network, n_pos, network->opt_long_array );
}

/*****************************************************************************************
*  Name:		build_random_network_default
*  Description: Builds a new random network
******************************************************************************************/
void build_random_network_default( model *model )
{
	if( model->random_network->daily_fraction < 1e-9 )
		return;

	long n_pos, person;
	int jdx;
	long *interactions = model->possible_interactions;

	n_pos            = 0;
	for( person = 0; person < model->params->n_total; person++ )
		for( jdx = 0; jdx < model->population[person].random_interactions; jdx++ )
			interactions[n_pos++]=person;

	build_random_network( model, model->random_network, n_pos, interactions );
}

/*****************************************************************************************
*  Name:		add_interactions_from_network
*  Description: Adds the daily interactions to all individual from a network
******************************************************************************************/
void add_interactions_from_network(
	model *model,
	network *network
)
{
	long idx     = 0;
	long inter_idx, inter_max;
	int day      = model->interaction_day_idx;
	int skip_hospitalised = network->skip_hospitalised;
	int skip_quarantined  = network->skip_quarantined;
	double prob_drop      = 1.0 - network->daily_fraction;
	interaction *inter1, *inter2;
	interaction_block *inter_block = model->interaction_blocks[day];
	individual *indiv1, *indiv2;

	if( network->daily_fraction < 1e-9 )
		return;

	// get the latest interaction block (drop the last one since need to get 2 at a time)
	while( inter_block->idx > ( inter_block->n_interactions -2 ) )
	{
		if( inter_block->next == NULL )
			print_exit( "run out of interactions tokens" );
		inter_block = inter_block->next;
	}
	inter_idx = inter_block->idx;
	inter_max = inter_block->n_interactions - 2;

	while( idx < network->n_edges )
	{
		indiv1 = &(model->population[ network->edges[idx].id1 ] );
		indiv2 = &(model->population[ network->edges[idx++].id2 ] );

		if( indiv1->status == DEATH || indiv2 ->status == DEATH )
			continue;
		if( skip_hospitalised && ( is_in_hospital( indiv1 ) || is_in_hospital( indiv2 ) ) )
			continue;
		if( prob_drop > 0 && gsl_ran_bernoulli( rng, prob_drop ) )
			continue;
		if( skip_quarantined && ( indiv1->quarantined || indiv2->quarantined ) && !model->params->soft_quarantine_on )
			continue;

		inter1 = &(inter_block->interactions[inter_idx++]);
		inter2 = &(inter_block->interactions[inter_idx++]);
		inter1->type       = network->type;
		inter1->network_id = network->network_id;
		inter1->traceable  = UNKNOWN;
		inter1->manual_traceable  = UNKNOWN;
		inter1->individual = indiv2;
		inter1->next       = indiv1->interactions[ day ];
		indiv1->interactions[ day ] = inter1;
		indiv1->n_interactions[ day ]++;

		inter2->type       = network->type;
		inter2->network_id = network->network_id;
		inter2->traceable  = UNKNOWN;
		inter2->manual_traceable  = UNKNOWN;
		inter2->individual = indiv1;
		inter2->next       = indiv2->interactions[ day ];
		indiv2->interactions[ day ] = inter2;
		indiv2->n_interactions[ day ]++;

		model->n_total_intereactions++;

		if( inter_idx > inter_max  )
		{
			inter_block->idx = inter_idx;
			while( inter_block->idx > ( inter_block->n_interactions -2 ) )
			{
				if( inter_block->next == NULL )
					print_exit( "run out of interactions tokens (2)" );
				inter_block = inter_block->next;
			}
			inter_idx = inter_block->idx;
			inter_max = inter_block->n_interactions - 2;
		}
	}
	inter_block->idx = inter_idx;
}

/*****************************************************************************************
*  Name:		build_daily_network
*  Description: Builds a new interaction network
******************************************************************************************/
void build_daily_network( model *model )
{
	int idx, day;
	network *user_network;

	day = model->interaction_day_idx;
	for( idx = 0; idx < model->params->n_total; idx++ )
		model->population[ idx ].n_interactions[ day ] = 0;

	build_random_network_default( model );
	add_interactions_from_network( model, model->random_network );
	add_interactions_from_network( model, model->household_network );

	for( idx = 0; idx < model->n_occupation_networks; idx++ )
		add_interactions_from_network( model, model->occupation_network[idx] );

	user_network = model->user_network;
	while( user_network != NULL )
	{
		if( user_network->construction == NETWORK_CONSTRUCTION_RANDOM )
			build_random_network_user( model, user_network );

		add_interactions_from_network( model, user_network );
		user_network = user_network->next_network;
	}

	if( model->params->hospital_on )
	{
		for( idx = 0; idx < model->params->n_hospitals; idx++ )
        {
            rebuild_healthcare_worker_patient_networks( model, &(model->hospitals[idx]) );
            add_hospital_network_interactions(  model, &(model->hospitals[idx]) );
        }
	}
}

/*****************************************************************************************
*  Name:		transition_events
*  Description: Transitions all people from one type of event
*  Returns:		void
******************************************************************************************/
void transition_events(
	model *model_ptr,
	int type,
	void (*transition_func)( model*, individual* ),
	int remove_event
)
{
	long idx, n_events;
	event *event, *next_event;
	individual *indiv;

	n_events    = model_ptr->event_lists[type].n_daily_current[ model_ptr->time ];
	next_event  = model_ptr->event_lists[type].events[ model_ptr->time ];

	for( idx = 0; idx < n_events; idx++ )
	{
		event      = next_event;
		next_event = event->next;
		indiv      = event->individual;
		transition_func( model_ptr, indiv );

		if( remove_event )
			remove_event_from_event_list( model_ptr, event );
	}
}

/*****************************************************************************************
*  Name:		transition_events_info
*  Description: Transitions all people from one type of event
*  Returns:		void
******************************************************************************************/
void transition_events_info(
	model *model_ptr,
	int type,
	void (*transition_func)( model*, individual*, void* ),
	int remove_event
)
{
	long idx, n_events;
	event *event, *next_event;
	individual *indiv;

	n_events    = model_ptr->event_lists[type].n_daily_current[ model_ptr->time ];
	next_event  = model_ptr->event_lists[type].events[ model_ptr->time ];

	for( idx = 0; idx < n_events; idx++ )
	{
		event      = next_event;
		next_event = event->next;
		indiv      = event->individual;
		transition_func( model_ptr, indiv, event->info );

		if( remove_event )
			remove_event_from_event_list( model_ptr, event );
	}
}

/*****************************************************************************************
*  Name:		set_up_healthcare_workers
*  Description: randomly pick individuals from population between ages 20 - 69 to be doctors
*               and nurses
*  Returns:		void
******************************************************************************************/
void set_up_healthcare_workers_and_hospitals( model *model)
{
	//TODO: Have set_up_healthcare_workers() mimic the age distribution of actual NHS workers.
	long pdx;
	int idx, n_total_doctors, n_total_nurses;
	individual *indiv;

	//Initialise hospitals.
	model->hospitals = calloc( model->params->n_hospitals, sizeof(hospital) );
	for( idx = 0; idx < model->params->n_hospitals; idx++ )
		initialise_hospital( &(model->hospitals[idx]), model->params, idx );

	idx = 0;

	//Randomly pick individuals from population between ages 20 - 69 to be doctors and assign them to a hospital.
	n_total_doctors = model->params->n_hcw_per_ward[COVID_GENERAL][DOCTOR] * model->params->n_wards[COVID_GENERAL];
	n_total_doctors += model->params->n_hcw_per_ward[COVID_ICU][DOCTOR] * model->params->n_wards[COVID_ICU];
	while( idx < n_total_doctors )
	{
		pdx = gsl_rng_uniform_int( rng, model->params->n_total );
		indiv = &(model->population[pdx]);

		if( !individual_eligible_to_become_healthcare_worker( indiv ) )
			continue;

		indiv->worker_type = DOCTOR;
		indiv->occupation_network = HOSPITAL_WORK_NETWORK;
		add_healthcare_worker_to_hospital( &(model->hospitals[0]), indiv, DOCTOR );
		idx++;
	}

	idx = 0;

	//Randomly pick individuals from population between ages 20 - 69 to be nurses and assign them to a hospital.
	n_total_nurses = model->params->n_hcw_per_ward[COVID_GENERAL][NURSE] * model->params->n_wards[COVID_GENERAL];
	n_total_nurses += model->params->n_hcw_per_ward[COVID_ICU][NURSE] * model->params->n_wards[COVID_ICU];
	while( idx < n_total_nurses )
	{
		pdx = gsl_rng_uniform_int( rng, model->params->n_total );
		indiv = &(model->population[pdx]);

		if( !individual_eligible_to_become_healthcare_worker( indiv ) )
			continue;

		indiv->worker_type = NURSE;
		indiv->occupation_network = HOSPITAL_WORK_NETWORK;
		add_healthcare_worker_to_hospital( &(model->hospitals[0]), indiv, NURSE );
		idx++;
	}
}

/*****************************************************************************************
*  Name:		add_user_network
*  Description: Creates a new user user network (destroy an old one if it exists)
*  Returns:		void
******************************************************************************************/
int add_user_network(
	model *model,
	int type,
	int skip_hospitalised,
	int skip_quarantined,
	int construction,
	double daily_fraction,
	long n_edges,
	long *edgeStart,
	long *edgeEnd,
	char *name
)
{
	long idx;
	long n_total   = model->params->n_total;
	int network_id;
	network *user_network;

	// check to see that the edges all make sense
	for( idx = 0; idx < n_edges; idx++ )
	{
		if( (edgeStart[ idx ] < 0) | (edgeEnd[ idx ] < 0) | (edgeStart[ idx ] >= n_total) | (edgeEnd[ idx ] >= n_total) )
		{
			print_now( "edgeStart and edgeEnd can only contain indices between 0 and n_total" );
			return FALSE;
		}
		if( edgeStart[ idx ] == edgeEnd[ idx ] )
		{
			print_now( "edgeStart and edgeEnd can't connect the same person " );
			return FALSE;
		}
	}

	// get the next free network_id
	network_id = model->n_occupation_networks + 1;
	user_network = model->user_network;
	while( user_network != NULL )
	{
		network_id   = max( network_id, user_network->network_id );
		user_network = user_network->next_network;
	}
	network_id++;

	user_network = create_network( model->params->n_total, type );
	user_network->edges = calloc(n_edges, sizeof(edge));
	user_network->n_edges = n_edges;
	user_network->skip_hospitalised = skip_hospitalised;
	user_network->skip_quarantined  = skip_quarantined;
	user_network->daily_fraction    = daily_fraction;
	user_network->network_id		= network_id;
	strcpy( user_network->name, name );

	for( idx = 0; idx < n_edges; idx++ )
	{
		user_network->edges[idx].id1 = edgeStart[ idx ];
		user_network->edges[idx].id2 = edgeEnd[ idx ];
	}

	user_network->next_network = model->user_network;
	model->user_network        = user_network;

	add_interaction_block( model, n_edges * 2 );

	model->rebuild_networks = TRUE;

	return network_id;
}

/*****************************************************************************************
*  Name:		add_user_network_random
*  Description: Creates a new random user user network
*  Arguments:	model  				- pointer to the model
*  				type   				- type of network
*  				skip_hospitalised 	- don't include people if hospitalised
*  				skip_quarantined    - don't include people if quarantined
*  				n_indiv				- total number of people to include
*  				pdxs				- the person idxs of each person on network (array length n_indiv)
*  				interactions		- the number of daily interactions for each person (array length n_indiv)
*  				name 				- the name of the network
*  Returns:		void
******************************************************************************************/
int add_user_network_random(
	model *model,
	int skip_hospitalised,
	int skip_quarantined,
	long n_indiv,
	long *pdxs,
	int *interactions,
	char *name
)
{
	long idx, total_interactions, n_edges;
	long n_total   = model->params->n_total;
	int network_id;
	network *user_network;

	// check to see that the  all make sense
	if( n_indiv > n_total )
	{
		print_now( "The number of people on the network must be less than total size of the population");
		return FALSE;
	}

	for( idx = 0; idx < n_indiv; idx++ )
	{
		if( (pdxs[ idx ] < 0) | (pdxs[ idx ] >= n_total) )
		{
			print_now( "pdxs must be between between 0 and n_total " );
			return FALSE;
		}
		if( (interactions[ idx ] < 0) )
		{
			print_now( "the number of daily interactions must be positive" );
			return FALSE;
		}
	}

	// get the next free network_id
	network_id = model->n_occupation_networks + 1;
	user_network = model->user_network;
	while( user_network != NULL )
	{
		network_id   = max( network_id, user_network->network_id );
		user_network = user_network->next_network;
	}
	network_id++;

	// set on the meta data of the new network
	user_network = create_network( model->params->n_total, RANDOM );
	user_network->skip_hospitalised = skip_hospitalised;
	user_network->skip_quarantined  = skip_quarantined;
	user_network->construction      = NETWORK_CONSTRUCTION_RANDOM;
	user_network->daily_fraction    = 1;
	user_network->network_id		= network_id;
	strcpy( user_network->name, name );

	// set on the people and the number of interactions
	user_network->opt_n_indiv   = n_indiv;
	user_network->opt_pdx_array = calloc( n_indiv, sizeof(long));
	user_network->opt_int_array = calloc( n_indiv, sizeof(int));
	total_interactions = 0;
	for( idx = 0; idx < n_indiv; idx++ )
	{
		user_network->opt_pdx_array[idx] = pdxs[idx];
		user_network->opt_int_array[idx] = interactions[idx];
		total_interactions += interactions[idx];
	}

	// allocate memory for the edges when they are calculated
	n_edges = ceil( total_interactions * 0.5);
	user_network->n_edges        = n_edges;
	user_network->n_vertices     = n_indiv;
	user_network->edges          = calloc( n_edges, sizeof( edge ) );
	user_network->opt_long       = total_interactions;
	user_network->opt_long_array = calloc( total_interactions, sizeof( long ) );

	// add to lust of networks and allocate memory for the interactions
	user_network->next_network = model->user_network;
	model->user_network        = user_network;
	add_interaction_block( model, total_interactions );

	model->rebuild_networks = TRUE;

	return network_id;
}

/*****************************************************************************************
*  Name:		delete_network
*  Description: removes a network from the model
*  				for user networks these are entirely removed
*  				for defulat network they are silenced by setting daily fraction to 0
*  Returns:		TRUE/FALSE
******************************************************************************************/
int delete_network( model *model, network *net )
{
	int idx;
	int default_network = FALSE;
	network *last_network = model->user_network;

	if( ( model->random_network == net ) ||
		( model->household_network == net ) )
		default_network = TRUE;

	for( idx = 0; idx < model->n_occupation_networks; idx++ )
		if( model->occupation_network[ idx ] == net )
			default_network = TRUE;

	if( default_network )
	{
		update_daily_fraction( net, 0.0 );
		return TRUE;
	}
	else
	{
		if( net == model->user_network )
		{
			model->user_network = net->next_network;
			destroy_network( net );
			return TRUE;
		}
		else
		{
			while( last_network->next_network != NULL )
			{
				if( last_network->next_network == net )
				{
					last_network = net->next_network;
					destroy_network( net );
					return TRUE;
				}
				last_network = last_network->next_network;
			}
		}
	}

	model->rebuild_networks = TRUE;

	return FALSE;
}



/*****************************************************************************************
*  Name:		get_network_by_id
*  Description: returns a pointer to a network with a given ID
*  Returns:		pointer to network
******************************************************************************************/
network* get_network_by_id( model *model, int network_id )
{
	int idx;

	if( model->random_network->network_id == network_id )
		return model->random_network;
	if( model->household_network->network_id == network_id )
		return model->household_network;

	for( idx = 0; idx < model->n_occupation_networks; idx++ )
		if( model->occupation_network[ idx ]->network_id == network_id )
			return model->occupation_network[ idx ];

	network *user_network = model->user_network;
	while( user_network != NULL )
	{
		if( user_network->network_id == network_id )
			return user_network;
		user_network = user_network->next_network;
	};

	return NULL;
}

/*****************************************************************************************
*  Name:		get_network_ids
*  Description: gets all the network ids
*  				network ids are set on the array pointer
*  Returns:		the number of ids
******************************************************************************************/
int get_network_ids( model *model, int *ids, int max_ids )
{
	int idx;
	int n_ids = 0;
	network *user_network;

	ids[ n_ids++ ] = model->household_network->network_id;

	for( idx = 0; idx < model->n_occupation_networks; idx++ )
	{
		if( n_ids == max_ids )
			return -1;
		ids[ n_ids++ ] = model->occupation_network[ idx ]->network_id;
	}

	if( n_ids == max_ids )
		return -1;
	ids[ n_ids++ ] = model->random_network->network_id;

	user_network = model->user_network;
	while( user_network != NULL )
	{
		if( n_ids == max_ids )
			return -1;
		ids[ n_ids++ ] = user_network->network_id;
		user_network   = user_network->next_network;
	}
	return( n_ids );
}

/*****************************************************************************************
*  Name:		get_network_id_by_index
*  Description: gets a network ids by index
*  Returns:		the network id
******************************************************************************************/
int get_network_id_by_index( model *model, int idx )
{
	int offset = 0, remainder;
	network *user_network;

  if( idx < 0 )
    print_exit("idx (=%d) must be greater than 0", idx);

  if( idx == 0 )
    return( model->household_network->network_id );
  offset += 1;

  if( idx < (offset + model->n_occupation_networks) )
    return( model->occupation_network[idx - offset]->network_id );
  offset += model->n_occupation_networks;

  if( idx == offset )
    return( model->random_network->network_id );
  offset += 1;

  remainder = idx - offset;
	user_network = model->user_network;
	while( user_network != NULL && remainder > 0 )
  {
		user_network = user_network->next_network;
    remainder--;
  }

  if( user_network != NULL )
    return( user_network->network_id );

  return( -1 );
}

/*****************************************************************************************
*  Name:		return_interactions
*  Description: returns all the interaction which are being dropped from peoples
*  				interaction diary to the the stack of usable tokens
*  Returns:		void
******************************************************************************************/
void return_interactions( model *model )
{
	int day_idx  = model->interaction_day_idx;
	interaction_block *inter_block = model->interaction_blocks[day_idx];

	inter_block->idx = 0;
	while( inter_block->next != NULL )
	{
		inter_block = inter_block->next;
		inter_block->idx = 0;
	}

	return;
}

/*****************************************************************************************
*  Name:		one_time_step
*  Description: Move the model through one time step
*  Returns:     int
******************************************************************************************/
int one_time_step( model *model )
{
	//printf("llllllllllllllll\t");
	if (DEBUG) printf("\n");
	(model->time)++;
	if (DEBUG)
		printf("Starting day t = %d\n",  model->time);
	reset_counters( model );
	update_intervention_policy( model, model->time );

	int idx;
	for( idx = 0; idx < N_EVENT_TYPES; idx++ )
		update_event_list_counters( model, idx );

	if( model->rebuild_networks ){
		ring_inc( model->interaction_day_idx, model->params->days_of_interactions );
		return_interactions( model );

		build_daily_network( model );
		model->rebuild_networks = model->params->rebuild_networks;
	}
	individual *indiv = &(model->population[0]);
	if (DEBUG) printf("\nSTART t = %d, nov = %d, status = %d, q = %d, cl = %d, h = %d, lna = %d/%d/%d/%d\n", model->time, indiv->app_user, indiv->status, indiv->quarantined, get_caution_level(model, indiv), is_in_hospital(indiv), indiv->caution_until[0], indiv->caution_until[1], indiv->caution_until[2], indiv->caution_until[3]);

	transmit_virus( model );
	new_seed_infections( model );

	if (DEBUG) printf("MID   t = %d, nov = %d, status = %d, q = %d, cl = %d, h = %d, lna = %d/%d/%d/%d\n", model->time, indiv->app_user, indiv->status, indiv->quarantined, get_caution_level(model, indiv), is_in_hospital(indiv), indiv->caution_until[0], indiv->caution_until[1], indiv->caution_until[2], indiv->caution_until[3]);

	transition_events( model, SYMPTOMATIC,       	   &transition_to_symptomatic,      		FALSE );
	transition_events( model, SYMPTOMATIC_MILD,  	   &transition_to_symptomatic_mild, 		FALSE );
	transition_events( model, HOSPITALISED,     	   &transition_to_hospitalised,     		FALSE );
	transition_events( model, CRITICAL,          	   &transition_to_critical,         		FALSE );
	transition_events( model, HOSPITALISED_RECOVERING, &transition_to_hospitalised_recovering,  FALSE );
	transition_events( model, RECOVERED,         	   &transition_to_recovered,        		FALSE );
	transition_events( model, SUSCEPTIBLE,			   &transition_to_susceptible,				FALSE );
	transition_events( model, DEATH,             	   &transition_to_death,            		FALSE );

	if( model->params->hospital_on )
	{
		transition_events( model, DISCHARGED,      		   &transition_to_discharged, 				FALSE );
		transition_events( model, MORTUARY,        		   &transition_to_mortuary,   				FALSE );

		swap_waiting_general_and_icu_patients( model );
		hospital_waiting_list_transition_scheduler( model, GENERAL );
		hospital_waiting_list_transition_scheduler( model, ICU );

		transition_events( model, WAITING,         &transition_to_waiting,    FALSE );
		transition_events( model, GENERAL,         &transition_to_general,    FALSE );
		transition_events( model, ICU,             &transition_to_icu,        FALSE );
    }

	flu_infections( model );

	while( ( n_daily( model, TEST_TAKE, model->time ) > 0 ) ||
		   ( n_daily( model, TEST_RESULT, model->time ) > 0 ) ||
		   ( n_daily( model, MANUAL_CONTACT_TRACING, model->time ) > 0 )
	)
	{
		transition_events( model, TEST_TAKE,              &intervention_test_take,          TRUE );
		transition_events( model, TEST_RESULT,            &intervention_test_result,        TRUE );
		transition_events( model, MANUAL_CONTACT_TRACING, &intervention_manual_trace,       TRUE );
	}

	transition_events_info( model, VACCINE_PROTECT,          &intervention_vaccine_protect, TRUE );
	transition_events_info( model, VACCINE_WANE,             &intervention_vaccine_wane, TRUE );

	transition_events( model, QUARANTINE_RELEASE,     &intervention_quarantine_release, FALSE );
	//printf("B q = %d, sq = %d, mult=%lf/%lf/%lf/%lf\n", indiv->quarantined, model->params->soft_quarantine_on, model->params->novid_soft_multiplier[0], model->params->novid_soft_multiplier[1], model->params->novid_soft_multiplier[2], model->params->novid_soft_multiplier[3]);
	transition_events( model, TRACE_TOKEN_RELEASE,    &intervention_trace_token_release,FALSE );
	//printf("C q = %d\n", indiv->quarantined);

	if( model->params->quarantine_smart_release_day > 0 )
		intervention_smart_release( model );

	model->n_quarantine_days += model->event_lists[QUARANTINED].n_current;
	if (DEBUG) {
		long freq[5];
		for (int i = 0; i<5; i++)
			freq[i] = 0;
		for (long i = 0; i<model->params->n_total; i++)
			freq[get_caution_level(model, &(model->population[i]))]++;
		for (int i = 0; i<5; i++)
			printf("%ld\t", freq[i]);
		printf("\t\tllllllllllllllllllll\n");
	}
	if (DEBUG) printf("END   t = %d, nov = %d, status = %d, q = %d, cl = %d, h = %d, lna = %d/%d/%d/%d\n", model->time, indiv->app_user, indiv->status, indiv->quarantined, get_caution_level(model, indiv), is_in_hospital(indiv), indiv->caution_until[0], indiv->caution_until[1], indiv->caution_until[2], indiv->caution_until[3]);

	if (DEBUG) printf("\n");
	return 1;
}
