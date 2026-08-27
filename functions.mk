### ============================================================================
### Small cross-platform makefile helpers
### ============================================================================

ifeq ($(OS),Windows_NT)
    OS_NAME := Windows
    EXE_EXT := .exe
    MKDIR = mkdir
    RM = del /q

    define log
        @echo (INFO)    ${1}
    endef

    define header
        @echo ================= [ ${1} ] =================
        @echo (INFO)      OS: $(OS_NAME)
        @echo (INFO)    HOST: $(shell $(CC) -dumpmachine)
        @echo (INFO)    HASH: $(GIT_HASH)
        @echo (INFO)    Compile starting [$(CC)]
    endef

    define success
        @echo (SUCCESS) ${1}
    endef

    define warn
        @echo (WARNING) ${1}
    endef
else
    OS_NAME := $(shell uname -s 2>/dev/null || echo Unix)
    EXE_EXT :=
    MKDIR = mkdir -p
    RM = rm -f

    define log
        @printf '%s\n' '(INFO)    ${1}'
    endef

    define header
        @printf '%s\n' '================= [ ${1} ] ================='
        @printf '%s\n' '(INFO)      OS: $(OS_NAME)'
        @printf '%s\n' '(INFO)    HOST: $(shell $(CC) -dumpmachine)'
        @printf '%s\n' '(INFO)    HASH: $(GIT_HASH)'
        @printf '%s\n' '(INFO)    Compile starting [$(CC)]'
    endef

    define success
        @printf '%s\n' '(SUCCESS) ${1}'
    endef

    define warn
        @printf '%s\n' '(WARNING) ${1}'
    endef
endif
