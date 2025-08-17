#!/bin/bash

GEM5_EXEC="./build/RISCV/gem5.opt"
CONFIG_SCRIPT="configs/deprecated/example/se.py"

L1D_SIZE="32kB"
L1I_SIZE="32kB"
L2_SIZE="256kB"
CACHELINE_SIZE="64"


for f in {0..617}; do
    $GEM5_EXEC \
        --stats-file="vector-opt-_$f.txt" \
    $CONFIG_SCRIPT \
    --cmd="/gem5/mycode/tokenize/vector-tokenize" \
    --option="-p /gem5/mycode/tokenize -f wikitext-103/output_$f.txt" \
    --caches \
    --l2cache \
    --l1d_size=$L1D_SIZE \
    --l1i_size=$L1I_SIZE \
    --l2_size=$L2_SIZE \
    --cacheline_size=$CACHELINE_SIZE
done
