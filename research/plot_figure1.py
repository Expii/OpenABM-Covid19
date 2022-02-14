#!/usr/bin/env python

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument('--seed', help='common random seed', required=False, default=-1, type=int)
parser.add_argument('--output', help='output_path', required=False, default='output.png')
args = parser.parse_args()

from base import seeds 

if args.seed != -1:
    seeds = [args.seed]

N_TOTAL = 100000
RESULTS_DIR = "results/figure1/"
MAX_RSCALED = 201

def get_overall_r_infected(adoption, ten_times_r, novid, seed):
    if novid:
        return 100 * np.load(RESULTS_DIR + f"novid_{adoption}_{ten_times_r}_{seed}_overall.npy")[-1] / N_TOTAL
    else:
        return 100 * np.load(RESULTS_DIR + f"non_novid_{adoption}_{ten_times_r}_{seed}_overall.npy")[-1] / N_TOTAL

non_70 = np.mean(np.stack([np.array([get_overall_r_infected(70, x, False, seed) for x in range(0, MAX_RSCALED)]) for seed in seeds]), axis=0)
non_100 = np.mean(np.stack([np.array([get_overall_r_infected(100, x, False, seed) for x in range(0, MAX_RSCALED)]) for seed in seeds]), axis=0)

nov_70 = np.mean(np.stack([np.array([get_overall_r_infected(70, x, True, seed) for x in range(0, MAX_RSCALED)]) for seed in seeds]), axis=0)
nov_100 = np.mean(np.stack([np.array([get_overall_r_infected(100, x, True, seed) for x in range(0, MAX_RSCALED)]) for seed in seeds]), axis=0)

x = np.arange(0, MAX_RSCALED) / 10

plt.figure(figsize=(9,6))
ax = plt.subplot(111)

if args.seed == -1:
    ax.set_title('Total Infections in First 200 Days For Varying R0 \n Under Ideal Conditions (i.e. Full Caution at Distances 1, 2)', fontdict={'fontsize': 14})
else:
    ax.set_title(f'Total Infections in First 200 Days For Varying R0 (seed = {args.seed})\n Under Ideal Conditions (i.e. Full Caution at Distances 1, 2)', fontdict={'fontsize': 14})
ax.set_xlabel('R0', fontdict={'fontsize': 12})
ax.set_ylabel('Percentage Infected (incl. Recovered)', fontdict={'fontsize': 12})

ax.plot(x, [100] * len(x), color='k')
ax.plot(x, non_70, '-', color='tab:red', label="Contact Tracing, 70% Adoption")
ax.plot(x, non_100, '-', color='tab:orange', label="Contact Tracing, 100% Adoption")
ax.plot(x, nov_70, '-', color='tab:green', label="NOVID, 70% Adoption")
ax.plot(x, nov_100, '-', color='tab:blue', label="NOVID, 100% Adoption")

plt.legend( prop={'size': 14})

plt.savefig(args.output)
