#!/usr/bin/python3

import pandas as pd
import numpy as np
from argparse import ArgumentParser

from base import phone_adoption, change_adoption, run_with_modifications, turn_off_novid, change_caution

def main():
    parser = ArgumentParser()
    parser.add_argument('--adoption', help='app adoption rate', required=True, type=int)
    parser.add_argument('--output_path', help='output folder and prefix (_user.npy and _overall.npy are added)', required=True)
    parser.add_argument('--R', help='R value', required=False, type=float, default=5.8*1.6)
    parser.add_argument('--seed', help='seed for RNG', required=False, type=int, default=23649)
    parser.add_argument('--novid', help='NOVID used?', action='store_true')
    parser.add_argument('--phone', help='Scale app adoption according to phone ownership', action='store_true')
    args = parser.parse_args()
    
    if args.phone:
        new_params = phone_adoption(args.adoption/100)
    else:
        new_params = change_adoption(args.adoption/100)

    new_params['infectious_rate'] = args.R
    new_params['rng_seed'] = args.seed

    if args.novid:
        results = run_with_modifications(change_caution([0.0, 0.0, 1.0], new_params))
    else:
        results = run_with_modifications(turn_off_novid(new_params))
    
    results.to_csv(args.output_path + '_full.csv')
        
    results_overall = results['total_infected']
    results_user = results['n_app_user_infected']

    np.save(args.output_path + '_overall.npy', results_overall)
    if args.adoption > 0:
        np.save(args.output_path + '_user.npy', results_user)

if __name__ == "__main__":
    main()
