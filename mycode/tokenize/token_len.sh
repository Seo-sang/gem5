#!/bin/bash

EXEC="./x86-tokenize"

for f in {0..617}; do
    $EXEC \
    -p ~/research/gem5/mycode/tokenize \
    -f wikitext-103/output_$f.txt
done
