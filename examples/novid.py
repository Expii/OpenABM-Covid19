"""
Run the model with NOVID replacing the contact tracing app

Created: June 21, 2021
Author: Howard Halim
"""

import COVID19.model as abm
import pandas as pd

# get the model overiding a couple of params
model = abm.Model( params = {
    "n_total": 10000,
    "end_time": 100,
    "rng_seed": 2,

    "test_on_symptoms": 1,
    "testing_symptoms_time_on": 1,
    "trace_on_positive": 0,
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
} )

# run the model
model.run()

# print the basic output
pd.set_option('display.max_rows', None)
print( model.results[['total_infected', 'n_presymptom', 'n_asymptom', 'n_quarantine', 'n_tests', 'n_symptoms', 'n_hospital', 'n_hospitalised_recovering', 'n_critical', 'n_death', 'n_recovered']])
#print(model.results.columns.values)

