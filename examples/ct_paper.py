'''
Run the model with NOVID replacing the contact tracing app

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
]

caution_idx = 2
#app_adoption = 56
app_adoption = 72.27
test_prob = 0.8
test_delay = 0
quarantine_time = 14
app_report_prob = 1.0
n_seed_infection = 1000

start = time.time()

# get the model overiding a couple of params
model = abm.Model( params = {
    'n_total': 100000,
    'end_time': 200,
    'rng_seed': 38396,
    'n_seed_infection': n_seed_infection,

    'self_quarantine_fraction': test_prob,
    #'daily_non_cov_symptoms_rate': 0,
    'mean_time_to_symptoms': 5.42 + test_delay,

    'app_users_fraction_0_9':   0.09 * app_adoption / 72.27,
    'app_users_fraction_10_19': 0.80 * app_adoption / 72.27,
    'app_users_fraction_20_29': 0.97 * app_adoption / 72.27,
    'app_users_fraction_30_39': 0.96 * app_adoption / 72.27,
    'app_users_fraction_40_49': 0.94 * app_adoption / 72.27,
    'app_users_fraction_50_59': 0.86 * app_adoption / 72.27,
    'app_users_fraction_60_69': 0.70 * app_adoption / 72.27,
    'app_users_fraction_70_79': 0.48 * app_adoption / 72.27,
    'app_users_fraction_80':    0.32 * app_adoption / 72.27,
    #'household_app_adoption': 1,
    #'soft_quarantine_household': 1, # max household caution

    # do we still need these?
    'trace_on_symptoms': 1,
    'quarantine_on_traced': 1,
    'tracing_network_depth': 1,
    'app_turn_on_time': 1,
    #'traceable_interaction_fraction': 1,
    #'quarantine_compliance_traced_symptoms': 1,
    #'quarantine_dropout_traced_symptoms': 0,
    #'quarantine_length_traced_symptoms': quarantine_time,
    
    'soft_quarantine_on': 1,
    'novid_soft_multiplier_1': caution_multipliers[caution_idx][0],
    'novid_soft_multiplier_2': caution_multipliers[caution_idx][1],
    'novid_soft_multiplier_3': caution_multipliers[caution_idx][2],
    'novid_soft_multiplier_4': caution_multipliers[caution_idx][3],
    'novid_quarantine_length': quarantine_time,
    
    'manual_trace_on': 1,
    'manual_trace_time_on': 1,
    'manual_trace_on_hospitalization': 1,
    'manual_trace_on_positive': 1,
    'manual_trace_n_workers': 1000000,
    'manual_trace_interviews_per_worker_day': 6,
    'manual_trace_notifications_per_worker_day': 12,
    'manual_traceable_fraction_household': 1.0,
    'manual_traceable_fraction_occupation': 1.0,
    'manual_traceable_fraction_random': 0.0,
    'novid_report_manual_traced': app_report_prob,
    'manual_trace_delay': 1,
} )

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
total_infected = [n_seed_infection] + total_infected
delta_infected = [total_infected[i] - total_infected[i-1] for i in range(1, len(total_infected))]
print(delta_infected)
