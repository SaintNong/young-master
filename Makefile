### ============================================================================
### Config and flags
### ============================================================================

# Compiler and directories, you can switch these as you wish.
CC = clang
SRC_DIR = src
BIN_DIR = bin
SPRT_DIR = $(BIN_DIR)/sprt

# Source file and target
SRC = $(wildcard $(SRC_DIR)/*.c)
EXE = Young_Master

# Git commit hash and short timestamp for auto-versioning
GIT_HASH := $(shell git rev-parse --short HEAD)
DEF_COMMIT_HASH := -DGIT_HASH=\""$(GIT_HASH)"\"
SPRT_NAME ?= $(shell date +"%Y-%m-%d-%H%M")

# Sanitized and assertion binaries
SAN_EXE := $(EXE)-sanitized
DBG_EXE := $(EXE)-assert

# SPRT binary
SPRT_EXE := $(EXE)-$(SPRT_NAME)

# Compiler flags
OPTIMIZE = -O3 -flto -march=native
POPCNT = -msse3 -mpopcnt
NDEBUG = -D'NDEBUG=1' 
WARN = -Wall -Werror -Wextra -Wshadow
LIBS = -lm

# For sanitized build
SANITIZE = -fsanitize=address,leak,undefined

# Default flags
CFLAGS = -std=c11 $(OPTIMIZE) $(POPCNT) $(WARN) $(DEF_COMMIT_HASH)

### ============================================================================
### ANSI Pretty Print Functions
### ============================================================================

define log
    @echo "\e[1;97m (INFO)    \e[0;97m ${1}\e[0m"
endef

define header
    @echo "\e[97m ================= [ \e[1;97m ${1} \e[0;97m ] ================= \e[0m"
	$(call log, HASH: \e[0;36m$(GIT_HASH)\e[0m)
    $(call log, TIME: $(shell date +'%Y-%m-%d %r'))
	$(call log, Compile starting [$(CC)])
endef

define success
    @echo "\e[1;92m (SUCCESS) \e[0;92m ${1} ✓\e[0m"
endef

define warn
    @echo "\e[1;33m (WARNING) \e[0;33m${1}\e[0m"
endef

### ============================================================================
### Targets
### ============================================================================

.PHONY: all default release run clean

# We default to release
default: release

# Fastest build
release: $(BIN_DIR)
	$(call header, Release Build: $(EXE))
	$(CC) $(SRC) $(NDEBUG) $(CFLAGS) $(LIBS) -o $(EXE)
	$(call success, Binary $(EXE) compiled)

# Builds with asserts on
assert: $(BIN_DIR)
	$(call header, Debug Build: $(DBG_EXE))
	$(call warn, Assertions are turned on so performance will be impacted in this build.)
	$(CC) $(SRC) $(NDEBUG) $(CFLAGS) $(LIBS) -o $(BIN_DIR)/$(DBG_EXE)
	$(call success, Binary $(BIN_DIR)/$(DBG_EXE) compiled)

# Builds with sanitizers
sanitize: $(BIN_DIR)
	$(call header, Sanitized Build: $(SAN_EXE))
	$(call warn, Sanitizers are turned on so performance will be impacted in this build.)
	$(CC) $(SRC) $(NDEBUG) $(SANITIZE) $(CFLAGS) $(LIBS) -o $(BIN_DIR)/$(SAN_EXE)
	$(call success, Binary $(BIN_DIR)/$(SAN_EXE) compiled)

# Builds binary for sprt testing vs the last version
sprt: $(SPRT_DIR)
	$(call header, SPRT Build: $(SPRT_EXE))
	$(CC) $(SRC) $(NDEBUG) $(CFLAGS) $(LIBS) -o $(SPRT_DIR)/$(SPRT_EXE)
	$(call success, Binary $(SPRT_DIR)/$(SPRT_EXE) compiled)
	$(call log, Starting SPRT Test)
	./sprt.sh
	$(call success, SPRT Test finished without errors)

$(SPRT_DIR):
	$(call log, Making directory: $(SPRT_DIR))
	mkdir -p $(SPRT_DIR)

$(BIN_DIR):
	$(call log, Making directory: $(BIN_DIR))
	mkdir -p $(BIN_DIR)

clean:
	$(call log, Cleaning...)
	rm -f $(BIN_DIR)/$(EXE)
	rm -f $(BIN_DIR)/$(SAN_EXE)
	rm -f $(BIN_DIR)/$(DBG_EXE)
