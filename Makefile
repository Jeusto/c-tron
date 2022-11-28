CC = gcc
CFLAGS = -Wall -Wextra -g -pedantic
LFLAGS = -lncurses
BUILDDIR = build
SRCDIR = src

all: create_build_dir client server

create_build_dir:
	mkdir -p $(BUILDDIR)

client: create_build_dir
	$(CC) $(CFLAGS) -o ./$(BUILDDIR)/client ./$(SRCDIR)/client.c $(LFLAGS)

client_test_1:
	./$(BUILDDIR)/client 0.0.0.0 5555 1

client_test_2:
	./$(BUILDDIR)/client 0.0.0.0 5555 2

server: create_build_dir
	$(CC) $(CFLAGS) -o ./$(BUILDDIR)/server ./$(SRCDIR)/server.c $(LFLAGS)

server_test:
	./$(BUILDDIR)/server 5555 300

clean:
	rm -rf build