#!/bin/bash
# Runs an SPRT against the latest young master and his previous generations.

set -e

### ============================================================================
### SPRT Config
### ============================================================================

SPRT_ELO0="0" # H0: New version is the same elo
SPRT_ELO1="5" # H1: New version is at at least 5 elo stronger
SPRT_ALPHA="0.05"
SPRT_BETA="0.10"

### ============================================================================
### Fastchess and Engine Config
### ============================================================================

# Directories
BIN_DIR="bin/sprt"
LOG_DIR="logs"

mkdir -p "$LOG_DIR"

# Tournament settings
CONC=14
ROUNDS=10000
GAMES_PER_ROUND=2
OPENINGS_BOOK="../books/8mvs.epd"

# Engine settings
ENGINE_NAME="Young Master"
TIME_CONTROL="10+0.1"
HASH_SIZE=64

# Get the latest version, and previous version to test
LATEST=$(ls -t "$BIN_DIR/"* | head -n1)
PREVIOUS=$(ls -t "$BIN_DIR/"* | tail -n +2 | head -n1)

# Generate names
LATEST_NAME="$(basename "$LATEST") (latest)"
PREVIOUS_NAME="$(basename "$PREVIOUS") (previous)"
TEST_NAME="${LATEST_NAME} vs ${PREVIOUS_NAME}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="$LOG_DIR/sprt_${TEST_NAME}_${TIMESTAMP}.log"


### ============================================================================
### Fastchess command
### ============================================================================

echo "===================================================================="
echo "SPRT Test: $TEST_NAME"
echo "SPRT Setting: elo0=$SPRT_ELO0 elo1=$SPRT_ELO1 alpha=$SPRT_ALPHA beta=$SPRT_BETA"
echo "Time Control: $TIME_CONTROL"
echo "Concurrency: $CONC"
echo "===================================================================="

FASTCHESS_CMD=(
    fastchess
    -engine "cmd=$LATEST" "name=$LATEST_NAME" "tc=$TIME_CONTROL"
    "option.Hash=$HASH_SIZE"
    -engine "cmd=$PREVIOUS" "name=$PREVIOUS_NAME" "tc=$TIME_CONTROL"
    "option.Hash=$HASH_SIZE"
    -openings "file=$OPENINGS_BOOK" format=epd order=random
    -sprt "elo0=$SPRT_ELO0" "elo1=$SPRT_ELO1" "alpha=$SPRT_ALPHA" "beta=$SPRT_BETA" model=normalized
    -rounds "$ROUNDS"
    -games "$GAMES_PER_ROUND"
    -concurrency "$CONC"
    -tournament gauntlet
    -autosaveinterval 10
    -report penta=true
    -draw movenumber=40 movecount=4 score=10
    -resign movecount=10 score=1200
    -log "file=$LOG_FILE" level=warn
)

echo "Fastchess command:"
echo "${FASTCHESS_CMD[@]}"
"${FASTCHESS_CMD[@]}"
