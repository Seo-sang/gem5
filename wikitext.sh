#!/bin/bash

GEM5_EXEC="./build/RISCV/gem5.opt"
CONFIG_SCRIPT="./configs/deprecated/example/se.py"

CMDS=(
  #  "/gem5/mycode/tokenize/scalar-tokenize-gc"
 #   "/gem5/mycode/tokenize/scalar-tokenize-gcv"
#    "/gem5/mycode/tokenize/vector-tokenize-seg"
    "/gem5/mycode/tokenize/rv64gcv-tokenize"
)



STAT_FILES=(
    #"scalar-gc-"
    #"scalar-gcv-"
    #"vector-seg-"
   # "scalar-model-load-"
   # "vector-model-load-"
    "rv64gcv-tokenize-"
)

L1D_SIZE="32kB"
L1I_SIZE="32kB"
L2_SIZE="256kB"
CACHELINE_SIZE="64"

CACHE_ENABLE=${1:-0}

if [ "$CACHE_ENABLE" -eq 1 ]; then
    for f in {1..98}; do
        for i in {0..0}; do #
            $GEM5_EXEC \
            --stats-file=${STAT_FILES[i]}"_$f.txt" \
            $CONFIG_SCRIPT \
            --cpu-type=RiscvMinorCPU \
            --cmd=${CMDS[i]} \
            --option="-p /gem5/mycode/tokenize -f wikitext-103/input_$f.txt" \
            --caches \
            --l2cache \
            --l1d_size=$L1D_SIZE \
            --l1i_size=$L1I_SIZE \
            --l2_size=$L2_SIZE \
            --cacheline_size=$CACHELINE_SIZE \
            >> "/gem5/mycode/tokenize/result/rv64gcv-ksc-$f.txt"
        done
    done
else
    for i in {0..2}; do
        for f in {0..1}; do
            $GEM5_EXEC \
            --stats-file="no_cache_"${STAT_FILES[i]}"stat_$f.txt" \
            $CONFIG_SCRIPT \
            --cmd=${CMDS[i]} \
            --option="-p /gem5/mycode/tokenize -f wikitext-103/output_$f.txt"
        done
    done
fi
