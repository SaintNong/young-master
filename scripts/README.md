# Evaluation tuning

```sh
make check-tuner-parity
make tune
make check
```

The parity check should pass before tuning to ensure the eval function in the tuner is the same as the engine. `make tune` replaces
`src/eval_weights.h` with the tuned values.

Paths and the parity limit can be overridden without editing a script:

```sh
make check-tuner-parity DATASET=/path/to/data.epd PARITY_LIMIT=10000
make tune TUNER_ROOT=/path/to/texel-tuner
```

For a new evaluation term, add the weight to `eval_weights.h`, use it in
`eval.c`, and add the matching trace, parameter, and output entry to the tuner.
Then run the parity check again.
