#!/bin/sh
set -eu

if [ "$#" -lt 3 ]; then
    echo "Usage: $0 ENGINE TUNER DATASET [LIMIT]" >&2
    exit 2
fi

engine=$1
tuner=$2
dataset=$3
limit=${4:-0}
tmp_dir=$(mktemp -d)
trap 'rm -rf -- "$tmp_dir"' EXIT HUP INT TERM

"$engine" eval-file "$dataset" "$limit" | grep '^EVAL ' > "$tmp_dir/engine"
"$tuner" --eval-file "$dataset" "$limit" > "$tmp_dir/tuner"
cmp "$tmp_dir/engine" "$tmp_dir/tuner"

count=$(wc -l < "$tmp_dir/engine")
echo "Evaluator parity passed for $count positions."
