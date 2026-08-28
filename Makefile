include functions.mk

### ============================================================================
### Config and flags
### ============================================================================

# Compiler and directories, you can switch these as you wish.
CC = clang
SRC_DIR = src
BIN_DIR = bin

# Source file and target
SRC = $(wildcard $(SRC_DIR)/*.c)
EXE = Young_Master

# Git commit hash and short timestamp for auto-versioning
GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
DEF_COMMIT_HASH := -DGIT_HASH=\""$(GIT_HASH)"\"

# Sanitized and assertion binaries
SAN_EXE := $(EXE)-sanitized
DBG_EXE := $(EXE)-assert

# Compiler flags
OPTIMIZE = -O3 -flto -march=native
POPCNT = -msse3 -mpopcnt
NDEBUG = -D'NDEBUG=1' 
WARN = -Wall -Werror -Wextra -Wshadow
LIBS = -lm

# For sanitized build
SANITIZE = -fsanitize=address,undefined

# Default flags
CFLAGS = -std=c11 $(OPTIMIZE) $(POPCNT) $(WARN) $(DEF_COMMIT_HASH)


### ============================================================================
### Targets
### ============================================================================

.PHONY: all default release assert sanitize check tune check-tuner-parity clean
.SUFFIXES:

# We default to release
default: release
all: release

# Fastest build
release: $(BIN_DIR)
	$(call header, Release Build: $(EXE))
	$(CC) $(SRC) $(NDEBUG) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(EXE)$(EXE_EXT)
	$(call success, Binary $(EXE) compiled)

# Builds with asserts on
assert: $(BIN_DIR)
	$(call header, Debug Build: $(DBG_EXE))
	$(call warn, Assertions are turned on so performance will be impacted in this build.)
	$(CC) $(SRC) $(CFLAGS) -UNDEBUG $(LDFLAGS) $(LIBS) -o $(BIN_DIR)/$(DBG_EXE)$(EXE_EXT)
	$(call success, Binary $(BIN_DIR)/$(DBG_EXE) compiled)

# Builds with sanitizers
sanitize: $(BIN_DIR)
	$(call header, Sanitized Build: $(SAN_EXE))
	$(call warn, Sanitizers are turned on so performance will be impacted in this build.)
	$(CC) $(SRC) $(CFLAGS) -UNDEBUG $(SANITIZE) $(LDFLAGS) $(LIBS) -o $(BIN_DIR)/$(SAN_EXE)$(EXE_EXT)
	$(call success, Binary $(BIN_DIR)/$(SAN_EXE) compiled)

check: sanitize release
	./$(BIN_DIR)/$(SAN_EXE)$(EXE_EXT) perft-test
	./$(BIN_DIR)/$(SAN_EXE)$(EXE_EXT) see-test
	./$(EXE)$(EXE_EXT) bench

TUNER_ROOT ?= ../texel-tuner
TUNER = $(TUNER_ROOT)/src/tuner
TUNING_SOURCES ?= $(TUNER_ROOT)/sources.csv
DATASET ?= ../data/quiet-labeled.v7.epd
PARITY_LIMIT ?= 0

tune:
	$(MAKE) -C $(TUNER_ROOT)/src
	cd $(TUNER_ROOT) && ./src/tuner "$(abspath $(TUNING_SOURCES))" "$(abspath src/eval_weights.h)"

check-tuner-parity: release
	$(MAKE) -C $(TUNER_ROOT)/src
	./scripts/check_tuner_parity.sh "./$(EXE)$(EXE_EXT)" "$(TUNER)" "$(DATASET)" "$(PARITY_LIMIT)"

$(BIN_DIR):
	$(call log, Making directory: $(BIN_DIR))
	$(MKDIR) $(BIN_DIR)

clean:
	$(call log, Cleaning...)
	-$(RM) $(EXE)$(EXE_EXT)
	-$(RM) $(BIN_DIR)/$(SAN_EXE)$(EXE_EXT)
	-$(RM) $(BIN_DIR)/$(DBG_EXE)$(EXE_EXT)
