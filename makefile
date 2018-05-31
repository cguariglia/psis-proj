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
	echo "Usage: make [server/lib/app]"

lib: $(LIBDIR)/clipboard.o

dirs_server:
	mkdir -p $(BINDIR) $(LIBDIR)

dirs_apps:
	mkdir -p $(APPDIR)/$(BINDIR) $(APPDIR)/$(LIBDIR) $(LIBDIR)

server: dirs_server lib $(LIBDIR)/server_global.o $(LIBDIR)/sync_protocol.o $(LIBDIR)/server_threads.o $(LIBDIR)/server.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(BINDIR)/$@ $(LDLIBS) $(wordlist 3, 6, $^)

app: dirs_apps lib $(APPDIR)/$(LIBDIR)/app_teste.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(word 3,$^) $(LIBDIR)/clipboard.o -o $(APPDIR)/$(BINDIR)/$@

.PHONY: clean

clean:
	rm -rf $(LIBDIR) $(BINDIR) $(APPDIR)/$(BINDIR) $(APPDIR)/$(LIBDIR)


inet: inet_server_test.c inet_client_test.c
	gcc -Wall -Wextra -pedantic -o server inet_server_test.c
	gcc -Wall -Wextra -pedantic -o client inet_client_test.c

cleaninet:
	rm server client
