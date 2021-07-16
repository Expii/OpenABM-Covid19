'''
These parameters match my simpler model

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

caution_idx = 3

start = time.time()

# get the model overiding a couple of params
model = abm.Model( params = {
    'n_total': 160000,
    'end_time': 200,
    'rng_seed': 3,
    'n_seed_infection': 160,
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
    'mean_time_to_recover': 200,
    'mean_asymptomatic_to_recovery': 200,
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
print(list(model.results['total_infected']))
