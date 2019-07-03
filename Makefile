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

install: pexec
	cp -v ./pexec "$(PREFIX)/bin"

clean:
	rm pexec -vf
