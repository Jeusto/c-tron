#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "../include/common.h"

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

void update_display() {}

int convert_key_to_movement(char c) {
  int key = -1;

  switch (c) {
    case 'z':
    case 'i':
      key = UP;
      break;
    case 's':
    case 'k':
      key = DOWN;
      break;
    case 'q':
    case 'j':
      key = LEFT;
      break;
    case 'd':
    case 'l':
      key = RIGHT;
      break;
    case ' ':
    case 'm':
      key = TRAIL_UP;
      break;
    default:
      break;
  }

  return key;
}

// arguments temporairement hardcodés
#define BUF_SIZE 1024
#define SERVER_PORT 5555
#define NB_JOUEURS_SUR_CLIENT 2

int main(int argc, char** argv) {
  struct sockaddr_in6 server_addr;
  int socket_fd, max_fd;
  int activity = 0;
  int byte_count = 0;
  fd_set readfds;
  display_info info = {
      .winner = -1,
      .board = {{0}},
  };
  // temporaire
  (void)argv;
  (void)argc;

  // init display
  // tune_terminal();
  // init_graphics();
  // srand(time(NULL));

  // creer socket
  CHECK(socket_fd = socket(AF_INET6, SOCK_STREAM, 0));  // todo: utiliser ipv4
  max_fd = socket_fd;

  // preparer adresse serveur
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(SERVER_PORT);
  server_addr.sin6_addr = in6addr_any;

  // se connecter au serveur
  CHECK(
      connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)));

  // envoyer nombre de joueurs sur ce client au serveur
  struct client_init_infos init_info = {
      .nb_players = NB_JOUEURS_SUR_CLIENT,
  };
  CHECK(send(socket_fd, &init_info, sizeof(uint32_t), 0));

  // TEMPORAIRE pour tester le display
  for (size_t x = 0; x < XMAX; x++) {
    for (size_t y = 0; y < YMAX; y++) {
      if (x == 0 || x == XMAX - 1 || y == 0 || y == YMAX - 1) {
        info.board[x][y] = 111;
      } else {
        info.board[x][y] = 120;
      }
    }
  }

  // boucle pour attendre messages du serveur
  while (1) {
    // preparer select
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(socket_fd, &readfds);

    CHECK(activity = select(max_fd + 1, &readfds, NULL, NULL, NULL));

    // message recu du serveur -> recuperer structure display_info et afficher
    if (FD_ISSET(socket_fd, &readfds)) {
      CHECK(byte_count =
                recv(socket_fd, &info, sizeof(struct display_info), 0));
      update_display();
    }

    // un des joueurs a appuyé sur une touche
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
      char buf[BUF_SIZE];
      CHECK(byte_count = read(STDIN_FILENO, buf, BUF_SIZE));
      buf[byte_count] = '\0';
      int player_id = 0;

      // todo: refactor cette chiasse
      if (NB_JOUEURS_SUR_CLIENT == 2 && buf[0] == 'z' || buf[0] == 'q' ||
          buf[0] == 's' || buf[0] == 'd' || buf[0] == ' ') {
        player_id = 0;
      } else if (buf[0] == 'i' || buf[0] == 'j' || buf[0] == 'k' ||
                 buf[0] == 'l' || buf[0] == 'm') {
        player_id = 1;
      }

      // envoyer touche au serveur
      struct client_input input = {
          .id = player_id,
          .input = convert_key_to_movement(buf[0]),
      };
      CHECK(send(socket_fd, &input, sizeof(struct client_input), 0));
    }
  }

  CHECK(close(socket_fd));
  return 0;
}
