#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

#define XMAX 100
#define YMAX 50
#define NB_COLORS 5
#define TRAIL_INDEX_SHIFT 50

#define BLUE_ON_BLACK 0
#define RED_ON_BLACK 2
#define YELLOW_ON_BLACK 1
#define MAGENTA_ON_BLACK 3
#define CYAN_ON_BLACK 4

#define BLUE_ON_BLUE 50
#define RED_ON_RED 52
#define YELLOW_ON_YELLOW 51
#define MAGENTA_ON_MAGENTA 53
#define CYAN_ON_CYAN 54

void tune_terminal() {
  struct termios term;
  tcgetattr(0, &term);
  term.c_iflag &= ~ICANON;
  term.c_lflag &= ~ICANON;
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &term);
}

void init_graphics() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(100);
  start_color();
  init_pair(BLUE_ON_BLACK, COLOR_BLUE, COLOR_BLACK);
  init_pair(RED_ON_BLACK, COLOR_RED, COLOR_BLACK);
  init_pair(YELLOW_ON_BLACK, COLOR_YELLOW, COLOR_BLACK);
  init_pair(MAGENTA_ON_BLACK, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(CYAN_ON_BLACK, COLOR_CYAN, COLOR_BLACK);

  init_pair(BLUE_ON_BLUE, COLOR_BLUE, COLOR_BLUE);
  init_pair(RED_ON_RED, COLOR_RED, COLOR_RED);
  init_pair(YELLOW_ON_YELLOW, COLOR_YELLOW, COLOR_YELLOW);
  init_pair(MAGENTA_ON_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA);
  init_pair(CYAN_ON_CYAN, COLOR_CYAN, COLOR_CYAN);

  init_pair(WALL, COLOR_WHITE, COLOR_WHITE);
}

void display_character(int color, int y, int x, char character) {
  attron(COLOR_PAIR(color));
  mvaddch(y, x, character);
  attroff(COLOR_PAIR(color));
}

// arguments temporairement hardcodés
#define BUF_SIZE 1024
#define NB_JOUEURS_SUR_CLIENT 2
#define SERVER_PORT 5555

int main(int argc, char **argv) {
  struct sockaddr_in6 server_addr;
  int socket_fd, activity, max_fd;
  int byte_count = 0;
  fd_set readfds;

  // creer socket
  CHECK(socket_fd = socket(AF_INET6, SOCK_STREAM, 0));
  max_fd = socket_fd;

  // preparer adresse serveur
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(SERVER_PORT);
  server_addr.sin6_addr = in6addr_any;

  // se connecter au serveur
  CHECK(
      connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)));

  // envoyer nombre de joueurs sur ce client au serveur
  uint32_t some_long = 10;
  uint32_t network_byte_order;
  network_byte_order = htonl(NB_JOUEURS_SUR_CLIENT);
  CHECK(send(socket_fd, &network_byte_order, sizeof(uint32_t), 0));

  tune_terminal();
  init_graphics();
  srand(time(NULL));

  for (;;) {
    clear();
    for (size_t i = 0; i < YMAX; i++) {
      for (size_t j = 0; j < XMAX; j++) {
        if (i == 0 || i == YMAX - 1) {
          display_character(CYAN_ON_CYAN, i, j, ' ');
        } else if (j == 0 || j == XMAX - 1) {
          display_character(CYAN_ON_CYAN, i, j, ' ');
        } else {
          display_character(RED_ON_RED, i, j, ' ');
          // if (rand() % 2)
          //   display_character(rand() % NB_COLORS, i, j, 'X');
          // else
          //   display_character(rand() % NB_COLORS + TRAIL_INDEX_SHIFT, i, j,
          //                     'X');
        }
      }
      mvaddstr(0, XMAX / 2 - strlen("C-TRON") / 2, "C-TRON");
    }

    sleep(2);
    refresh();
  }

  // boucle pour attendre messages du serveur
  while (1) {
    // preparer select
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(socket_fd, &readfds);

    // printf("En attente de messages, ecrire pour envoyer: \n");
    // on s'en fout des fd write et exception et le delai
    CHECK(activity = select(max_fd + 1, &readfds, NULL, NULL, NULL));

    if (FD_ISSET(socket_fd, &readfds)) {
      // message recu du serveur ->  afficher pour l'utilisateur
      char buf[BUF_SIZE];
      CHECK(byte_count = recv(socket_fd, buf, BUF_SIZE, 0));
      buf[byte_count] = '\0';
      printf("Recu %d bytes: %s\n", byte_count, buf);
    }
  }

  // fermeture socket
  CHECK(close(socket_fd));
  return 0;
}
