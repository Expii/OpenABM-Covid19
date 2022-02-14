#!/usr/bin/env python
# coding: utf-8

import COVID19.model as abm
from COVID19.model import VaccineSchedule
from COVID19.vaccine import Vaccine
import pandas as pd
import numpy as np
from argparse import ArgumentParser

soft_quaratine_on = 1
novid_on = 1
household_app_adoption = 1
cluster_app_adoption = 1

default_params = {
    'n_total': 100000,
    'end_time': 200,
    'rng_seed': 23649,
    'n_seed_infection': 0,
    'new_seed_infection_rate': 1,
    
    'self_quarantine_fraction': 0.8,
    'infectious_rate': 5.8 * 1.6, # *1.6 for delta 
    'mean_time_to_symptoms': 5.42 + 2, #+2 days for test delay
    
    'novid_on': 1,
    'app_users_fraction_0_9': 0.09 * 0.5 / 0.7227, # assuming 50% adoption for now
    'app_users_fraction_10_19': 0.80 * 0.5 / 0.7227, 
    'app_users_fraction_20_29': 0.97 * 0.5 / 0.7227,
    'app_users_fraction_30_39': 0.96 * 0.5 / 0.7227,
    'app_users_fraction_40_49': 0.94 * 0.5 / 0.7227,
    'app_users_fraction_50_59': 0.86 * 0.5 / 0.7227,
    'app_users_fraction_60_69': 0.70 * 0.5 / 0.7227,
    'app_users_fraction_70_79': 0.48 * 0.5 / 0.7227,
    'app_users_fraction_80':    0.32 * 0.5 / 0.7227,
    
    'household_app_adoption': 1,
    'cluster_app_adoption': 1,
    'soft_quarantine_household': 1,
    
    'trace_on_symptoms': 1,
    'quarantine_on_traced': 1,
    'tracing_network_depth': 1,
    'app_turn_on_time': 1,

    'soft_quarantine_on': 1,
    'novid_soft_multiplier_1': 0.125,
    'novid_soft_multiplier_2': 0.125,
    'novid_soft_multiplier_3': 0.25,
    'novid_soft_multiplier_4': 0.5,
    'novid_quarantine_length': 7,

    'manual_trace_on': 1,
    'manual_trace_time_on': 10000 * 0,
    'manual_trace_on_hospitalization': 1,
    'manual_trace_on_positive': 1,
    'manual_trace_n_workers': 1000000,
    'manual_trace_interviews_per_worker_day': 6,
    'manual_trace_notifications_per_worker_day': 12,
    'manual_traceable_fraction_household': 1.0,
    'manual_traceable_fraction_occupation': 1.0,
    'manual_traceable_fraction_random': 0.0,
    'manual_trace_delay': 1,

    'novid_report_manual_traced': 0.2,
}

seeds = [23649, 36492, 64923, 49236, 92364]

def phone_adoption(new_adoption_rate, other_changes={}):
    new_changes = other_changes.copy()
    new_changes['app_users_fraction_0_9'] = 0.09 * new_adoption_rate / 0.7227
    new_changes['app_users_fraction_10_19'] = 0.80 * new_adoption_rate / 0.7227
    new_changes['app_users_fraction_20_29'] = 0.97 * new_adoption_rate / 0.7227
    new_changes['app_users_fraction_30_39'] = 0.96 * new_adoption_rate / 0.7227
    new_changes['app_users_fraction_40_49'] = 0.94 * new_adoption_rate / 0.7227
    new_changes['app_users_fraction_50_59'] = 0.86 * new_adoption_rate / 0.7227
    new_changes['app_users_fraction_60_69'] = 0.70 * new_adoption_rate / 0.7227
    new_changes['app_users_fraction_70_79'] = 0.48 * new_adoption_rate / 0.7227
    new_changes['app_users_fraction_80'] = 0.32 * new_adoption_rate / 0.7227
    return new_changes

def change_adoption(new_adoption_rate, other_changes={}):
    new_changes = other_changes.copy()
    new_changes['app_users_fraction_0_9'] = new_adoption_rate
    new_changes['app_users_fraction_10_19'] = new_adoption_rate
    new_changes['app_users_fraction_20_29'] = new_adoption_rate
    new_changes['app_users_fraction_30_39'] = new_adoption_rate
    new_changes['app_users_fraction_40_49'] = new_adoption_rate
    new_changes['app_users_fraction_50_59'] = new_adoption_rate
    new_changes['app_users_fraction_60_69'] = new_adoption_rate
    new_changes['app_users_fraction_70_79'] = new_adoption_rate
    new_changes['app_users_fraction_80'] = new_adoption_rate
    return new_changes

def turn_off_novid(other_changes={}):
    new_changes = other_changes.copy()
    new_changes['novid_soft_multiplier_3'] = 1.0
    new_changes['novid_soft_multiplier_4'] = 1.0
    new_changes['manual_trace_on'] = 0
    new_changes['manual_trace_time_on'] = 10000
    return new_changes

def change_caution(caution_multipliers, other_changes={}):
    new_changes = other_changes.copy()
    new_changes['novid_soft_multiplier_2'] = caution_multipliers[0]
    new_changes['novid_soft_multiplier_3'] = caution_multipliers[1]
    new_changes['novid_soft_multiplier_4'] = caution_multipliers[2]
    return new_changes

def vaccinate(model):
    vaccine = model.add_vaccine(
        full_efficacy = [0.5],
        symptom_efficacy = [0.5],
        severe_efficacy = [0.9],
        time_to_protect = 14,
        vaccine_protection_period = 365
    )
    schedule = VaccineSchedule(
        frac_0_9 = 0,
        frac_10_19 = 0.42,
        frac_20_29 = 0.59,
        frac_30_39 = 0.61,
        frac_40_49 = 0.69,
        frac_50_59 = 0.77,
        frac_60_69 = 0.83,
        frac_70_79 = 0.86,
        frac_80 = 0.83,
        vaccine=vaccine
    )
    model.vaccine_schedule(schedule)

def run_with_modifications(changes={}, run_vaccinations=False):
    params = default_params.copy()
    params.update(changes)
    model = abm.Model(params=params)
    if run_vaccinations:
        vaccinate(model)
    model.run()
    return model.results

