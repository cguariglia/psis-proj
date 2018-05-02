# psis-proj
#  |_ bin
#  |_ include
#  |_ lib
#  |_ src
#  |_ [makefile]
#
# gotsa make it like this ^

CC=gcc
CFLAGS=-std=C11 -Wall -Wextra -pedantic
LDLIBS=
#LDFLAGS=-Linclude		# se usarmos aquela estrutura po projeto, descomenta-se isto

help usage:
	echo "Usage: make [apps/server]"

server: cbserver.o clipboard.o

apps: app_teste.o clipboard.o

app_teste.o: app_teste.c clipboard.h

cbserver.o: cbserver.c clipboard.h

clipboard.o: clipboard.c clipboard.h
