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
ifneq ($(OS),Windows_NT)
SPRT_NAME ?= $(shell date +"%Y-%m-%d-%H%M")
endif

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
SANITIZE = -fsanitize=address,undefined

# Default flags
CFLAGS = -std=c11 $(OPTIMIZE) $(POPCNT) $(WARN) $(DEF_COMMIT_HASH)


### ============================================================================
### Cross-platform time and mkdir/rm
### ============================================================================

# Date + Time
ifeq ($(OS),Windows_NT)
    DATE_TIME = $(shell powershell -Command "Get-Date -Format 'yyyy-MM-dd hh:mm:ss tt'")
else
    DATE_TIME = $(shell date +'%Y-%m-%d %r')
endif

# Working mkdir and rm
ifeq ($(OS),Windows_NT)
	CFLAGS += -static
	MKDIR = if not exist "$(1)" mkdir "$(1)"
	RM = if exist "$(1)" del /q "$(1)"
else
	MKDIR = mkdir -p "$(1)"
	RM = rm -f "$(1)"
endif


### ============================================================================
### Printing Functions
### ============================================================================

# Colour definitions, disable colours on Windows
ifeq ($(OS),Windows_NT)
    C_INFO    :=
    C_SUCCESS :=
    C_WARN    :=
    C_HEADER  :=
    C_RESET   :=
    C_DIM     :=
    C_CYAN    :=
else
    C_INFO    := \e[1;97m
    C_SUCCESS := \e[1;92m
    C_WARN    := \e[1;33m
    C_HEADER  := \e[97m
    C_RESET   := \e[0m
    C_DIM     := \e[0;97m
    C_CYAN    := \e[0;36m
endif

define log
	$(info $(C_INFO)(INFO)    $(C_DIM)${1}$(C_RESET))
endef

define header
	$(info $(C_HEADER)================= [ $(C_INFO)${1} $(C_HEADER)] ================= $(C_RESET))
	$(call log, HASH: $(C_CYAN)$(GIT_HASH)$(C_RESET))
	$(call log, TIME: $(DATE_TIME))
	$(call log, Compile starting [$(CC)])
endef

# Windows doesn't support checkmarks so this will have to do
ifeq ($(OS),Windows_NT)
define success
	@echo $(C_SUCCESS)(SUCCESS) $(C_DIM)${1} yay!$(C_RESET)
endef
else
define success
	@echo "$(C_SUCCESS)(SUCCESS) $(C_DIM)${1} ✓$(C_RESET)"
endef
endif

define warn
	$(info $(C_WARN)(WARNING) $(C_DIM)${1}$(C_RESET))
endef

### ============================================================================
### Targets
### ============================================================================

.PHONY: all default release assert sanitize sprt clean

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
# NOTE: (Not supported on Windows)
sanitize: $(BIN_DIR)
ifeq ($(OS),Windows_NT)
	$(call warn, This recipe is not supported on Windows)
else
	$(call header, Sanitized Build: $(SAN_EXE))
	$(call warn, Sanitizers are turned on so performance will be impacted in this build.)
	$(CC) $(SRC) $(NDEBUG) $(SANITIZE) $(CFLAGS) $(LIBS) -o $(BIN_DIR)/$(SAN_EXE)
	$(call success, Binary $(BIN_DIR)/$(SAN_EXE) compiled)
endif

# Builds binary for sprt testing vs the last version
# NOTE: (Not supported on Windows)
sprt: $(SPRT_DIR)
ifeq ($(OS),Windows_NT)
	$(call warn, This recipe is not supported on Windows)
else
	$(call header, SPRT Build: $(SPRT_EXE))
	$(CC) $(SRC) $(NDEBUG) $(CFLAGS) $(LIBS) -o $(SPRT_DIR)/$(SPRT_EXE)
	$(call success, Binary $(SPRT_DIR)/$(SPRT_EXE) compiled)
	$(call log, Starting SPRT Test)
	./sprt.sh
	$(call success, SPRT Test finished without errors)
endif

$(SPRT_DIR):
	$(call log, Making directory: $(SPRT_DIR))
	$(MKDIR) $(SPRT_DIR)

$(BIN_DIR):
	$(call log, Making directory: $(BIN_DIR))
	$(MKDIR) $(BIN_DIR)

clean:
	$(call log, Cleaning...)
	$(RM) $(BIN_DIR)/$(EXE)
	$(RM) $(BIN_DIR)/$(SAN_EXE)
	$(RM) $(BIN_DIR)/$(DBG_EXE)
