PREFIX ?= /usr/local
DESTDIR := 
BIN := pexec

CFLAGS ?= -O2
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic

CPPFLAGS += -D_GNU_SOURCE

.PHONY: all clean test install

all: pexec

test: pexec
	@bash ./test.sh

$(BIN): pexec.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

install: pexec
	install -Dm755 $(BIN) $(DESTDIR)/$(PREFIX)/bin/$(BIN)

clean:
	rm pexec -vf