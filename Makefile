CXXFLAGS ?= -O2
CXXFLAGS += -std=c++11 -Wall -Wextra -Wpedantic

.PHONY: all clean test

all: pexec

test: pexec
	bash ./test.sh

%: %.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm pexec -vf
