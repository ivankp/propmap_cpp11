CFLAGS := -std=c++11 -Wall -O3

.PHONY: all clean

EXE := test

all: $(EXE)

$(EXE): %: %.cc propmap.hh
	@echo CC $@
	@g++ $(CFLAGS) $^ -o $@

clean:
	rm -f $(EXE)
