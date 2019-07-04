SHELL := /bin/bash
PREFIX ?= /usr/local
CFLAGS ?= -O2
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS += -D_GNU_SOURCE

.PHONY: all clean test install

all: pexec

test: pexec
	@bash ./test.sh

%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

features/%.txt features/%.exe: features/%.c
	@echo -n "Checking for $*..." >&2; \
	if $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o features/$*.exe >/dev/null 2>/dev/null; then\
		echo "#define HAS_$(shell echo "$*" | tr a-z A-Z) 1" > $@; \
		echo "yes" >&2; \
	else \
		echo "" > $@; \
		echo "missing" >&2; \
	fi

feature_tests = $(shell find features | grep '\.c$$' | sed 's@\.c$$@.txt@')

pexec_features.h: $(feature_tests)
	@cat $(feature_tests) > $@

install: pexec
	cp -v ./pexec "$(PREFIX)/bin"

clean:
	rm pexec pexec_features.h features/*.txt features/*.exe -vf
