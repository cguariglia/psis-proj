# psis-proj
#  |_ bin
#  |_ include
#  |_ lib
#  |_ src
#  |_ [makefile]
#

CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic -g
CPPFLAGS=-Iinclude -D_POSIX_C_SOURCE="200809L" -D_DEFAULT_SOURCE
LDLIBS=-pthread

APPDIR=apps
BINDIR=bin
LIBDIR=lib
SRCDIR=src

$(LIBDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(APPDIR)/$(LIBDIR)/%.o: $(APPDIR)/$(SRCDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

help usage:
	echo "Usage: make [server/lib/apps/all]"

lib: $(LIBDIR)/clipboard.o

dirs_server:
	mkdir -p $(BINDIR) $(LIBDIR)

dirs_apps:
	mkdir -p $(APPDIR)/$(BINDIR) $(APPDIR)/$(LIBDIR) $(LIBDIR)

server: dirs_server lib $(LIBDIR)/server_global.o $(LIBDIR)/sync_protocol.o $(LIBDIR)/server_threads.o $(LIBDIR)/server.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(BINDIR)/$@ $(LDLIBS) $(wordlist 3, 6, $^)

copy: dirs_apps lib $(APPDIR)/$(LIBDIR)/copy.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(word 3,$^) $(LIBDIR)/clipboard.o -o $(APPDIR)/$(BINDIR)/copy

paste: dirs_apps lib $(APPDIR)/$(LIBDIR)/paste.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(word 3,$^) $(LIBDIR)/clipboard.o -o $(APPDIR)/$(BINDIR)/paste
	
wait: dirs_apps lib $(APPDIR)/$(LIBDIR)/wait.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(word 3,$^) $(LIBDIR)/clipboard.o -o $(APPDIR)/$(BINDIR)/wait

print_clipboard: dirs_apps lib $(APPDIR)/$(LIBDIR)/print_clipboard.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(word 3,$^) $(LIBDIR)/clipboard.o -o $(APPDIR)/$(BINDIR)/print_clipboard

apps: copy paste wait print_clipboard

all: server apps

.PHONY: clean

clean:
	rm -rf $(LIBDIR) $(BINDIR) $(APPDIR)/$(BINDIR) $(APPDIR)/$(LIBDIR)
