#!/bin/bash

if [ ! -f "./othello" ]; then
    echo "othello executable not found in the current directory."
    exit 1
fi

if [ ! -f "default_input" ]; then
    echo "Input file not found in the current directory."
    exit 1
fi

RESULT_DIR="results"
mkdir -p $RESULT_DIR

echo "Running othello with CILK_NWORKERS set from 1 to 32..."

for n in {1..32}
do
    echo "Running with CILK_NWORKERS=$n..."

    (time CILK_NWORKERS=$n ./othello < default_input) &> "$RESULT_DIR/time_n_$n.txt"
done

echo "Done. Check the $RESULT_DIR directory for results."
