CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -pedantic # ajouter -Werror avant rendu
LFLAGS = -lncurses
BUILDDIR = build
OBJDIR = obj
SRCDIR = src
DEPS = $(OBJDIR)/display.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

create_dir:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)

client: $(OBJDIR)/client.o $(OBJDIR)/display.o
	$(CC) $(CFLAGS) -o ./$(BUILDDIR)/client $(OBJDIR)/client.o $(OBJDIR)/display.o $(LFLAGS)

server:
	$(CC) $(CFLAGS) -o ./$(BUILDDIR)/server ./$(SRCDIR)/server.c $(LFLAGS)

clean:
	rm -rf build