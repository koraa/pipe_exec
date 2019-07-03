PREFIX ?= /usr/local

CXXFLAGS ?= -O2
CXXFLAGS += -std=c++11 -Wall -Wextra -Wpedantic

CPPFLAGS += -D_GNU_SOURCE

.PHONY: all clean test install

all: pexec

test: pexec
	@bash ./test.sh

%: %.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

install: pexec
	cp -v ./pexec "$(PREFIX)/bin"

clean:
	rm pexec -vf
