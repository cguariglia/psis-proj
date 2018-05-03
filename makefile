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
LIBDIR=lib
SRCDIR=src

$(LIBDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

help usage:
	echo "Usage: make [server/lib/app]"

lib: $(LIBDIR)/clipboard.o

server: $(LIBDIR)/cbserver.o lib
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(BINDIR)/$@ $(LDLIBS)

app: app_teste.o lib
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(APPDIR)/$@ $(LDLIBS)

.PHONY: clean

clean:
	rm -f $(LIBDIR)/*.o
