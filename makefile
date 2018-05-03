# psis-proj
#  |_ bin
#  |_ include
#  |_ lib
#  |_ src
#  |_ [makefile]
#
# gotsa make it like this ^

CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic
CPPFLAGS=-Linclude
LDLIBS=

APPDIR=apps
BINDIR=bin
INCDIR=include
LIBDIR=lib
SRCDIR=src

SERVERDEPS=

help usage:
	echo "Usage: make [apps/server]"
#include/cbrequest.h
lib: clipboard.c clipboard.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

server: cbserver.o clipboard.o
	$(CC) $(CFLAGS)

apps: app_teste.o clipboard.o

app_teste.o: app_teste.c clipboard.h

cbserver.o: cbserver.c clipboard.h include/cbrequest.h

clipboard.o: clipboard.c clipboard.h include/cbrequest.h
