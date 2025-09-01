SRC = $(wildcard *.c)
FLAGS = -Wall -O3 -flto -march=native -ffast-math
LIBS = -lm
NO_DEBUG = -D'NDEBUG=1'

EXE = Empyrean
SPRT_DIR = ../sprt

default:
	make run

# no optimizations
noflags:
	gcc $(SRC) $(LIBS) -o $(EXE)

# compiles with asserts
debug:
	gcc $(SRC) $(FLAGS) $(LIBS) -o $(EXE)

# fastest compile
performance:
	gcc $(SRC) $(FLAGS) $(LIBS) $(NO_DEBUG) -o $(EXE)

# compiles with a version name for sprt testing
sprt:
	@if [ -z "$(VERSION)" ]; then \
		echo "Example: make sprt VERSION=eval_test"; \
		echo "Will create: ../sprt/empyrean_eval_test"; \
		exit 1; \
	fi
	mkdir -p $(SPRT_DIR)
	gcc $(SRC) $(FLAGS) $(LIBS) $(NO_DEBUG) -o $(SPRT_DIR)/$(EXE)_$(VERSION)

# compiles and runs
run:
	make performance
	./$(EXE)

clean:
	rm $(EXE)
