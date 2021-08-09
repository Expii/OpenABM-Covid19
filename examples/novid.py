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
    [0,0,0,0],
    [0.5, 0.5, 0.5, 0.5],
    [0, 0, 0.25, 0.5],
    [0.125, 0.125, 0.125, 0.125],
]

NOVID = 1

caution_idx = 2 * (1 - NOVID)
app_adoption = 0.5
test_prob = 0.8
test_delay = 2
quarantine_time = 7
app_report_prob = 0.2
n_seed_infection = 0
new_seed_infection_rate = 1
n_total = 100000
end_time = 200

start = time.time()

# original R_0 = 2.54
# delta R_0 = 3.56

# get the model overiding a couple of params
model = abm.Model( params = {
    'n_total': n_total,
    'end_time': end_time,
    'rng_seed': 23649,
    #'rng_seed': 5,
    'n_seed_infection': n_seed_infection,
    'new_seed_infection_rate': new_seed_infection_rate,
    #'infectious_rate': 2.4,

    'self_quarantine_fraction': test_prob,
    'infectious_rate': 5.8 * 1.6,
    #'daily_non_cov_symptoms_rate': 1,
    'mean_time_to_symptoms': 5.42 + test_delay,
    #'sd_time_to_symptoms': 0,

    'novid_on': 1,
    'app_users_fraction_0_9':   0.09 * app_adoption / 0.7227,
    'app_users_fraction_10_19': 0.80 * app_adoption / 0.7227,
    'app_users_fraction_20_29': 0.97 * app_adoption / 0.7227,
    'app_users_fraction_30_39': 0.96 * app_adoption / 0.7227,
    'app_users_fraction_40_49': 0.94 * app_adoption / 0.7227,
    'app_users_fraction_50_59': 0.86 * app_adoption / 0.7227,
    'app_users_fraction_60_69': 0.70 * app_adoption / 0.7227,
    'app_users_fraction_70_79': 0.48 * app_adoption / 0.7227,
    'app_users_fraction_80':    0.32 * app_adoption / 0.7227,

    #'app_users_fraction_0_9':    app_adoption,
    #'app_users_fraction_10_19':  app_adoption,
    #'app_users_fraction_20_29':  app_adoption,
    #'app_users_fraction_30_39':  app_adoption,
    #'app_users_fraction_40_49':  app_adoption,
    #'app_users_fraction_50_59':  app_adoption,
    #'app_users_fraction_60_69':  app_adoption,
    #'app_users_fraction_70_79':  app_adoption,
    #'app_users_fraction_80':     app_adoption,
    'household_app_adoption': 1,
    'cluster_app_adoption': 1,
    'soft_quarantine_household': 1, # max household caution

    'trace_on_symptoms': 1,
    'quarantine_on_traced': 1,
    'tracing_network_depth': 1,
    'app_turn_on_time': 1,

    'soft_quarantine_on': 1,
    'novid_soft_multiplier_1': caution_multipliers[caution_idx][0],
    'novid_soft_multiplier_2': caution_multipliers[caution_idx][1],
    'novid_soft_multiplier_3': caution_multipliers[caution_idx][2],
    'novid_soft_multiplier_4': caution_multipliers[caution_idx][3],
    'novid_quarantine_length': quarantine_time,

    'manual_trace_on': NOVID,
    'manual_trace_time_on': 10000 * (1 - NOVID),
    'manual_trace_on_hospitalization': 1,
    'manual_trace_on_positive': 1,
    'manual_trace_n_workers': 1000000,
    'manual_trace_interviews_per_worker_day': 6,
    'manual_trace_notifications_per_worker_day': 12,
    'manual_traceable_fraction_household': 1.0,
    'manual_traceable_fraction_occupation': 1.0,
    'manual_traceable_fraction_random': 0.0,
    'manual_trace_delay': 1,

    'novid_report_manual_traced': app_report_prob,  # TODO: rename (not just for manual tracing)
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
R = list(model.results['R_inst'])
for i in range(end_time):
    R[i] = round(R[i], 2)
print('\nR')
print(R)

print('\nTotal infected')
total_infected = list(model.results['total_infected'])
print(total_infected)

app_infected = list(model.results['n_app_user_infected'])
noapp_infected = [total_infected[i] - app_infected[i] for i in range(end_time)]
if app_adoption != 0:
    for i in range(end_time):
        app_infected[i] = round(app_infected[i] / (app_adoption * n_total) * 100, 2)
        noapp_infected[i] = round(noapp_infected[i] / ((1-app_adoption) * n_total) * 100, 2)

print('\nNon-app infected')
print(noapp_infected)

print('\nApp infected')
print(app_infected)

print('\nDaily infected')
total_infected = [n_seed_infection] + total_infected
delta_infected = [total_infected[i] - total_infected[i-1] for i in range(1, len(total_infected))]
print(delta_infected)

