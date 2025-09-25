### ============================================================================
### Ugly cross-platform makefile functions, put here in quarantine
### ============================================================================

# Platform detection
ifeq ($(OS),Windows_NT)

    ### ============================================================================
    ### Windows
    ### ============================================================================

    CFLAGS += -static
    
    # No colors on Windows
    C_INFO    :=
    C_SUCCESS :=
    C_WARN    :=
    C_HEADER  :=
    C_RESET   :=
    C_DIM     :=
    C_CYAN    :=


    # === Print functions (Windows) ===

    define log
        @echo (INFO)    ${1}
    endef

    define header
        @echo ================= [ ${1} ] =================
        @echo (INFO)      OS: $(OS)
        @echo (INFO)    HOST: $(shell $(CC) -dumpmachine)
        @echo (INFO)    HASH: $(GIT_HASH)
        @echo (INFO)    TIME: $(DATE_TIME)
        @echo (INFO)    Compile starting [$(CC)]
    endef

    define success
        @echo (SUCCESS) ${1} yay!
    endef

    define warn
        @echo (WARNING) ${1}
    endef

    # Date and time
	SPRT_NAME := 
    DATE_TIME = $(shell powershell -Command "Get-Date -Format 'yyyy-MM-dd hh:mm:ss tt'")

    # Windows compatible mkdir + rm
    MKDIR = mkdir
    RM = rm -Force
else
    ### ============================================================================
    ### Unix
    ### ============================================================================


    # Enable colors
    C_INFO    := \e[1;97m
    C_SUCCESS := \e[1;92m
    C_WARN    := \e[1;33m
    C_HEADER  := \e[97m
    C_RESET   := \e[0m
    C_DIM     := \e[0;97m
    C_CYAN    := \e[0;36m

    # === Print functions (Unix) ===

    define log
        @echo "$(C_INFO)(INFO)    $(C_DIM)${1}$(C_RESET)"
    endef

    define header
        @echo "$(C_HEADER)================= [ $(C_INFO)${1} $(C_HEADER)] ================= $(C_RESET)"
        @echo "$(C_INFO)(INFO)      $(C_DIM)OS: $(C_SUCCESS)$(OS)$(C_RESET)"
        @echo "$(C_INFO)(INFO)    $(C_DIM)HOST: $(C_CYAN)$(shell $(CC) -dumpmachine)$(C_RESET)"
        @echo "$(C_INFO)(INFO)    $(C_DIM)HASH: $(C_CYAN)$(GIT_HASH)$(C_RESET)"
        @echo "$(C_INFO)(INFO)    $(C_DIM)TIME: $(DATE_TIME)$(C_RESET)"
        @echo "$(C_INFO)(INFO)    $(C_DIM)Compile starting [$(CC)]$(C_RESET)"
    endef

    define success
        @echo "$(C_SUCCESS)(SUCCESS) $(C_DIM)${1} ✓$(C_RESET)"
    endef

    define warn
        @echo "$(C_WARN)(WARNING) $(C_DIM)${1}$(C_RESET)"
    endef

    # Date and time
	SPRT_NAME := $(shell date +"%Y-%m-%d-%H%M")
    DATE_TIME = $(shell date +'%Y-%m-%d %r')

    # mkdir and rm
    MKDIR = mkdir -p
    RM = rm -f
endif