#!/usr/bin/make

CC = gcc

default: writer

writer: writer.c
	$(CROSS_COMPILE)$(CC) $^ -o $@

clean:
	rm -f writer

.PHONY: clean
