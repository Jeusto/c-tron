CFLAGS = -Wall -Wextra -Werror -g -pedantic
LFLAGS = -lncurses
BUILDDIR = build
SRCDIR = src
INCLUDEDIR = include
PROGS = server client

all: create_build_dir $(PROGS)

server: $(BUILDDIR)/server.o $(BUILDDIR)/game-logic.o
		$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

client: $(BUILDDIR)/client.o $(BUILDDIR)/display.o
		$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
		$(CC) $(CFLAGS) -c -o $@ $<

create_build_dir:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(PROGS)