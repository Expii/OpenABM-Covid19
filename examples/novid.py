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
#    "lockdown_time_on": 1,
#    "lockdown_elderly_time_on": 1
} )

# run the model
model.run()

# print the basic output
pd.set_option('display.max_rows', None)
print( model.results[['total_infected', 'total_case', 'n_presymptom', 'n_asymptom', 'n_quarantine', 'n_tests', 'n_symptoms', 'R_inst']])
#print(model.results.columns.values)

