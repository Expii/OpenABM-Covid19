'''
These parameters match my simpler model, where people react on symptoms

Created: June 21, 2021
Author: Howard Halim
'''

import time
import COVID19.model as abm
import pandas as pd

caution_multipliers = [
    [0.125, 0.125, 0.25, 0.5],
    [0.125, 0.125, 1.0, 1.0],
    [0.0, 0.0, 1.0, 1.0],
    [1.0, 1.0, 1.0, 1.0],
    [0.125, 1.0, 0.25, 0.5],
    [0.0, 1.0, 1.0, 1.0],
]

caution_idx = 5
n_seed_infection = 1000
alertTime = 7
time_to_symptoms = 6
app_adoption = 0.3

start = time.time()

# get the model overiding a couple of params
model = abm.Model( params = {
    'n_total': 160000,
    'end_time': 200,
    'rng_seed': 44626,
    'n_seed_infection': n_seed_infection,
    'infectious_rate': 3,
    'household_size_1': 0,
    'household_size_2': 0,
    'household_size_3': 0,
    'household_size_4': 1,
    'household_size_5': 0,
    'household_size_6': 0,
    'population_0_9': 1,
    'population_10_19': 0,
    'population_20_29': 0,
    'population_30_39': 0,
    'population_40_49': 1,
    'population_50_59': 0,
    'population_60_69': 0,
    'population_70_79': 0,
    'population_80': 0,
    'daily_fraction_work': 1,
    'relative_transmission_household': 1,
    'mild_infectious_factor': 1,
    'asymptomatic_infectious_factor': 1,
    'mean_work_interactions_child': 10,
    'mean_work_interactions_adult': 10,
    'mean_work_interactions_elderly': 10,
    'mean_random_interactions_child': 0,
    'mean_random_interactions_adult': 0,
    'mean_random_interactions_elderly': 0,
    'relative_susceptibility_0_9': 1,
    'relative_susceptibility_10_19': 1,
    'relative_susceptibility_20_29': 1,
    'relative_susceptibility_30_39': 1,
    'relative_susceptibility_40_49': 1,
    'relative_susceptibility_50_59': 1,
    'relative_susceptibility_60_69': 1,
    'relative_susceptibility_70_79': 1,
    'relative_susceptibility_80': 1,
    'sd_infectiousness_multiplier': 0,
    'child_network_adults': 0,
    'elderly_network_adults': 0,
    'hospitalised_fraction_0_9': 0,
    'hospitalised_fraction_10_19': 0,
    'hospitalised_fraction_20_29': 0,
    'hospitalised_fraction_30_39': 0,
    'hospitalised_fraction_40_49': 0,
    'hospitalised_fraction_50_59': 0,
    'hospitalised_fraction_60_69': 0,
    'hospitalised_fraction_70_79': 0,
    'hospitalised_fraction_80': 0,
    'mean_time_to_recover': 300,
    'mean_asymptomatic_to_recovery': 300,
    'fraction_asymptomatic_0_9': 0,
    'fraction_asymptomatic_10_19': 0,
    'fraction_asymptomatic_20_29': 0,
    'fraction_asymptomatic_30_39': 0,
    'fraction_asymptomatic_40_49': 0,
    'fraction_asymptomatic_50_59': 0,
    'fraction_asymptomatic_60_69': 0,
    'fraction_asymptomatic_70_79': 0,
    'fraction_asymptomatic_80': 0,
    'mild_fraction_0_9': 1,
    'mild_fraction_10_19': 1,
    'mild_fraction_20_29': 1,
    'mild_fraction_30_39': 1,
    'mild_fraction_40_49': 1,
    'mild_fraction_50_59': 1,
    'mild_fraction_60_69': 1,
    'mild_fraction_70_79': 1,
    'mild_fraction_80': 1,

    'self_quarantine_fraction': 0.5,
    'quarantine_length_self': alertTime,
    'quarantine_dropout_self': 0,
    'mean_time_to_symptoms': time_to_symptoms,
    'sd_time_to_symptoms': 0,
    'daily_non_cov_symptoms_rate': 0,

    'novid_on': 1,
    'app_users_fraction_0_9': app_adoption,
    'app_users_fraction_10_19': app_adoption,
    'app_users_fraction_20_29': app_adoption,
    'app_users_fraction_30_39': app_adoption,
    'app_users_fraction_40_49': app_adoption,
    'app_users_fraction_50_59': app_adoption,
    'app_users_fraction_60_69': app_adoption,
    'app_users_fraction_70_79': app_adoption,
    'app_users_fraction_80': app_adoption,
    'soft_quarantine_household': 1, # max household caution
    'household_app_adoption': 1,

    'trace_on_symptoms': 1,
    'quarantine_on_traced': 1,
    'tracing_network_depth': 1,
    'app_turn_on_time': 1,
    'traceable_interaction_fraction': 1,
    'quarantine_compliance_traced_symptoms': 1,
    'quarantine_dropout_traced_symptoms': 0,
    'quarantine_length_traced_symptoms': alertTime,

    'soft_quarantine_on': 1,
    'novid_soft_multiplier_1': 0.125,
    'novid_soft_multiplier_2': 0.125,
    'novid_soft_multiplier_3': 1,
    'novid_soft_multiplier_4': 1,
    'novid_quarantine_length': alertTime,
})

mid = time.time()

# run the model
model.run()

end = time.time()

# print the basic output
pd.set_option('display.max_rows', None)
print( model.results[['total_infected', 'n_presymptom', 'n_asymptom', 'n_quarantine', 'n_tests', 'n_symptoms', 'n_hospital', 'n_recovered', 'R_inst']])
print(model.results.columns.values)

print(f'Time taken: {mid-start}, {end-mid} seconds')
print(list(model.results['R_inst']))
total_infected = list(model.results['total_infected'])
print(total_infected)
print(list(model.results['n_quarantine']))
total_infected = [n_seed_infection] + total_infected
delta_infected = [total_infected[i] - total_infected[i-1] for i in range(1, len(total_infected))]
print(delta_infected)
