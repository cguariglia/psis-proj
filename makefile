# psis-proj
#  |_ bin
#  |_ include
#  |_ lib
#  |_ src
#  |_ [makefile]
#

CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic
CPPFLAGS=-Iinclude
LDLIBS=

APPDIR=apps
BINDIR=bin
INCDIR=include
LIBDIR=lib
SRCDIR=src

SERVERDEPS=

$(LIBDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

help usage:
	echo "Usage: make [apps/server]"

lib: $(LIBDIR)/clipboard.o

server: $(LIBDIR)/cbserver.o $(LIBDIR)/clipboard.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(BINDIR)/$@ $(LDLIBS)

apps: app_teste.o clipboard.o

app_teste.o: app_teste.c clipboard.h

cbserver.o: cbserver.c clipboard.h include/cbrequest.h

clipboard.o: clipboard.c clipboard.h include/cbrequest.h
