CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -pedantic # ajouter -Werror avant rendu
LFLAGS = -lncurses
BUILDDIR = build
SRCDIR = src

all: create_build_dir client server

create_build_dir:
	mkdir -p $(BUILDDIR)

client:
	$(CC) $(CFLAGS) -o ./$(BUILDDIR)/client ./$(SRCDIR)/client.c $(LFLAGS)

server:
	$(CC) $(CFLAGS) -o ./$(BUILDDIR)/server ./$(SRCDIR)/server.c $(LFLAGS)

clean:
	rm -rf build