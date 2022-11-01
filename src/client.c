#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/display.h"

// arguments temporairement hardcodés
#define BUF_SIZE 1024
#define SERVER_PORT 5555
#define NB_JOUEURS_SUR_CLIENT 2

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

  // TEMPORAIRE pour tester le display
  // for (size_t x = 0; x < XMAX; x++) {
  //   for (size_t y = 0; y < YMAX; y++) {
  //     if (x == 0 || x == XMAX - 1 || y == 0 || y == YMAX - 1) {
  //       info.board[x][y] = 111;
  //     } else {
  //       info.board[x][y] = 120;
  //     }
  //   }
  // }
  // for (size_t x = 0; x < XMAX; x++) {
  //   for (size_t y = 0; y < YMAX; y++) {
  //     printf("%d ", info.board[x][y]);
  //   }
  //   printf("\n");
  // }
  // exit(1);

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
      printf("received board\n");

      for (int x = 0; x < XMAX; x++) {
        for (int y = 0; y < YMAX; y++) {
          printf("%d ", info.board[x][y]);
        }
        printf("\n");
      }
      // exit(1);

      // update_display();
    }

    // un des joueurs a appuyé sur une touche
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
      char buf[BUF_SIZE];
      CHECK(byte_count = read(STDIN_FILENO, buf, BUF_SIZE));
      buf[byte_count] = '\0';
      int player_id = 0;

      // TODO: refactor cette chiasse
      if (NB_JOUEURS_SUR_CLIENT == 2 && buf[0] == 'z' || buf[0] == 'q' ||
          buf[0] == 's' || buf[0] == 'd' || buf[0] == ' ') {
        player_id = 0;
      } else if (buf[0] == 'i' || buf[0] == 'j' || buf[0] == 'k' ||
                 buf[0] == 'l' || buf[0] == 'm') {
        player_id = 1;
      } else {
        continue;
      }

      // envoyer touche au serveur
      struct client_input input = {
          .id = player_id,
          .input = convert_key_to_movement(buf[0]),
      };
      printf("sending input %d to server, player %d, byte_count: %d\n",
             input.input, input.id, byte_count);
      CHECK(send(socket_fd, &input, sizeof(struct client_input), 0));
    }
  }

  CHECK(close(socket_fd));
  return 0;
}
