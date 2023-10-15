#!/usr/bin/make

CC = gcc
CFLAGS ?= -Wall -Werror

default: all

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $^ -o $@

clean:
	rm -f aesdsocket

install:
	$(INSTALL) -m755 aesdsocket $(DESTDIR)$(BINDIR)/aesdsocket

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/aesdsocket

test:
	./aesdsocket -d
	../assignment-autotest/test/assignment5/sockettest.sh
	killall aesdsocket

.PHONY: all install uninstall clean
