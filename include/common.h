#ifndef __COMMON_H__
#define __COMMON_H__

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define TRAIL_UP 4

#define SERV_PORT 5555
#define XMAX 100
#define YMAX 50
#define TIE -2
#define NO_WINNER -1
#define TRAIL_INDEX_SHIFT 50

#define SA struct sockaddr
#define SAI struct sockaddr_in
#define TV struct timeval

#define WALL 111
#define EMPTY ' '

#define BUF_SIZE 1024
#define MAX_PLAYERS 2

#define max(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })

#define CHECK(x)                                      \
  do {                                                \
    if ((x) == -1) {                                  \
      fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
      perror(#x);                                     \
      exit(EXIT_FAILURE);                             \
    }                                                 \
  } while (0)

typedef struct display_info {
  char board[XMAX][YMAX];
  int winner;
} display_info;

struct client_input {
  int id;
  char input;
};

struct client_init_infos {
  int nb_players;
};

#endif