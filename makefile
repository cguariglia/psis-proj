# for implicit rule stuff
CC=gcc
CFLAGS=-std=C11 -Wall -pedantic
LDLIBS=

server: clipboard.o cbserver.o

test:

cbserver.o: cbserver.c clipboard.h
	gcc -c -o $@ $CFLAGS

clipboard.o: clipboard.c clipboard.h
	gcc -c -o $@ $CFLAGS

.PHONY: clean

clean:
	rm -rf obj
