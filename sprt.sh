#!/bin/bash
# Runs an SPRT against the latest young master and his previous generations.

set -e

# Resolve paths relative to this script.
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

### ============================================================================
### SPRT Config
### ============================================================================

SPRT_ELO0="0" # H0: New version is the same elo
SPRT_ELO1="10" # H1: New version is at at least 10 elo stronger
SPRT_ALPHA="0.05"
SPRT_BETA="0.10"

### ============================================================================
### Fastchess and Engine Config
### ============================================================================

# Directories
BIN_DIR="$SCRIPT_DIR/bin/sprt"
LOG_DIR="$SCRIPT_DIR/logs"

mkdir -p "$LOG_DIR"

# Tournament settings
CONC=14
ROUNDS=10000
GAMES_PER_ROUND=2
OPENINGS_BOOK="$SCRIPT_DIR/../books/8mvs.epd"

# Engine settings
TIME_CONTROL="10+0.1"
HASH_SIZE=64

# Check prerequisites.
command -v make >/dev/null || { echo "make is required" >&2; exit 1; }
command -v fastchess >/dev/null || { echo "fastchess is required" >&2; exit 1; }
if [[ ! -f "$OPENINGS_BOOK" ]]; then
    echo "Opening book not found: $OPENINGS_BOOK" >&2
    exit 1
fi

# Select the previous staged binary.
mapfile -t PREVIOUS_ENGINES < <(
    find "$BIN_DIR" -maxdepth 1 -type f -name 'Young_Master-*' \
        -printf '%T@ %p\n' 2>/dev/null | sort -nr | cut -d' ' -f2-
)
if (( ${#PREVIOUS_ENGINES[@]} < 1 )); then
    echo "No previous engine found in $BIN_DIR" >&2
    echo "Run an SPRT once there is an earlier staged engine to compare against." >&2
    exit 1
fi
PREVIOUS="${PREVIOUS_ENGINES[0]}"

# Build and stage the current release.
make -C "$SCRIPT_DIR" release
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LATEST="$BIN_DIR/Young_Master-$TIMESTAMP"
cp "$SCRIPT_DIR/Young_Master" "$LATEST"
chmod +x "$LATEST"

# Generate names
LATEST_NAME="$(basename "$LATEST") (latest)"
PREVIOUS_NAME="$(basename "$PREVIOUS") (previous)"
TEST_NAME="${LATEST_NAME} vs ${PREVIOUS_NAME}"
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
