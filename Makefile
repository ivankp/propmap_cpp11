CFLAGS := -std=c++11 -Wall -g

.PHONY: all clean

EXE := test test_alloc

all: $(EXE)

$(EXE): %: %.cc propmap.hh
	@echo CC $@
	@g++ $(CFLAGS) $^ -o $@

clean:
	rm -f $(EXE)
