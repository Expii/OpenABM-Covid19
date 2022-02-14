#!/usr/bin/bash

START_TIME=`date +%s`
OUT_DIR=results/figure1

SEEDS=( 23649 36492 64923 49236 92364 )

mkdir -p $OUT_DIR

for seed in "${SEEDS[@]}"; do
	for ten_times_r in {0..201}; do
		R=$(bc -l <<< "scale=1; $ten_times_r/10")
		
		echo "doing simulations for R = $R and seed = $seed"
		python3 simulations_figure1.py --adoption 70 --output_path $OUT_DIR/non_novid_70_${ten_times_r}_${seed} --R $R --seed $seed
		python3 simulations_figure1.py --adoption 100 --output_path $OUT_DIR/non_novid_100_${ten_times_r}_${seed} --R $R --seed $seed
		python3 simulations_figure1.py --adoption 70 --output_path $OUT_DIR/novid_70_${ten_times_r}_${seed} --R $R --seed $seed --novid
		python3 simulations_figure1.py --adoption 100 --output_path $OUT_DIR/novid_100_${ten_times_r}_${seed} --R $R --seed $seed --novid
	done
	python3 plot_figure1.py --output $OUT_DIR/${seed}.png --seed $seed
done
python3 plot_figure1.py --output $OUT_DIR/overall.png
