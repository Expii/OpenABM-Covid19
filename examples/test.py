"""
Run the model with NOVID replacing the contact tracing app

Created: June 21, 2021
Author: Howard Halim
"""

caution_multipliers = [
    [0.125, 0.125, 0.25, 0.5],
    [0.125, 0.125, 1.0, 1.0],
    [0.0, 0.0, 1.0, 1.0],
    [1.0, 1.0, 1.0, 1.0],
    [0.125, 1.0, 0.25, 0.5],
]

caution_idx = 0

import COVID19.model as abm
import pandas as pd

# get the model overiding a couple of params
model = abm.Model( params = {
    "n_total": 10000,
    "end_time": 100,

    "self_quarantine_fraction": 0.0,
    "test_result_wait": 0,
    "test_order_wait": 0,
})

'''
    "n_total": 10000,
    "end_time": 100,
    "rng_seed": 1,

    "test_on_symptoms": 1,
    "testing_symptoms_time_on": 1,
    "trace_on_positive": 1,
    "app_turn_on_time": 1,
    "intervention_start_time": 1,
    "tracing_network_depth": 1,
    "quarantine_on_traced": 1,

    "self_quarantine_fraction": 0,
    "quarantine_household_on_symptoms": 0,
    "lockdown_elderly_time_on": 10000,

    "quarantine_dropout_self": 0,
    "quarantine_dropout_traced_symptoms": 0,
    "quarantine_dropout_traced_positive": 0,
    "quarantine_dropout_positive": 0,
    "quarantine_compliance_traced_symptoms": 1,
    "quarantine_compliance_traced_positive": 1,

    "test_release_on_negative": 0,
    "soft_quarantine_on": 1,
    "novid_quarantine_length": 7,

    "novid_phone_fraction": 1,
    "app_phone_fraction": 1,

    "novid_soft_multiplier_1": caution_multipliers[caution_idx][0],
    "novid_soft_multiplier_2": caution_multipliers[caution_idx][1],
    "novid_soft_multiplier_3": caution_multipliers[caution_idx][2],
    "novid_soft_multiplier_4": caution_multipliers[caution_idx][3],
} )
'''

# run the model
model.run()

# print the basic output
pd.set_option('display.max_rows', None)

#print( model.results[['total_infected', 'n_presymptom', 'n_asymptom', 'n_quarantine', 'n_tests', 'n_symptoms', 'n_hospital', 'n_critical', 'n_death', 'n_recovered', 'R_inst']])
print(model.results[['n_quarantine']])
#print(model.results.columns.values)

