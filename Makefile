CC = gcc
# CFLAGS = -Wall -Wextra -Werror -g -std=c99 -pedantic
CFLAGS = -Wall -Wextra -g -std=c99 -pedantic
LDFLAGS = -lncurses
EXEC = projet
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
BUILDDIR = build

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o
	rm -rf $(EXEC)
