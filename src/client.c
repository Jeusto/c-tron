#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/display.h"

int convert_key_to_movement(char c) {
  int input = -1;

  switch (c) {
    case KEY_UP_P1:
    case KEY_UP_P2:
      input = UP;
      break;
    case KEY_DOWN_P1:
    case KEY_DOWN_P2:
      input = DOWN;
      break;
    case KEY_LEFT_P1:
    case KEY_LEFT_P2:
      input = LEFT;
      break;
    case KEY_RIGHT_P1:
    case KEY_RIGHT_P2:
      input = RIGHT;
      break;
    case KEY_TRAIL_P1:
    case KEY_TRAIL_P2:
      input = TRAIL_UP;
      break;
    default:
      break;
  }

  return input;
}

int main(int argc, char** argv) {
  // TODO: utiliser ipv4 dans la version de base
  struct sockaddr_in6 server_addr;
  int socket_fd, max_fd;
  int activity = 0;
  int byte_count = 0;
  fd_set readfds;
  // TODO: temporaire
  (void)argv;
  (void)argc;

  // init display
  tune_terminal();
  init_graphics();
  srand(time(NULL));

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
      .nb_players = htonl(NB_JOUEURS_SUR_CLIENT),
  };
  CHECK(send(socket_fd, &init_info, sizeof(struct client_init_infos), 0));

  // boucle pour attendre messages du serveur et afficher le jeu
  while (1) {
    // preparer select
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(socket_fd, &readfds);

    // attendre activite
    CHECK(activity = select(max_fd + 1, &readfds, NULL, NULL, NULL));

    // message recu du serveur
    if (FD_ISSET(socket_fd, &readfds)) {
      display_info info;
      CHECK(byte_count =
                recv(socket_fd, &info, sizeof(struct display_info), 0));

      // serveur deconnecté
      if (byte_count == 0) {
        printf("server closed connection\n");
        break;
      }

      // mettre a jour l'affichage
      else {
        info.winner = ntohl(info.winner);
        // FIXME: temporaire pour tester mais a corriger
        info.board[0][0] = 111;

        clear();
        update_display(&info);
        refresh();
      }
    }

    // un des joueurs a appuyé sur une touche
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
      char buf[BUF_SIZE];
      CHECK(byte_count = read(STDIN_FILENO, buf, BUF_SIZE));
      buf[byte_count] = '\0';
      int player_id = 0;

      // TODO: refactor ca
      if (buf[0] == 'z' || buf[0] == 'q' || buf[0] == 's' || buf[0] == 'd' ||
          buf[0] == ' ') {
        player_id = 0;
      } else if (NB_JOUEURS_SUR_CLIENT == 2 && buf[0] == 'i' || buf[0] == 'j' ||
                 buf[0] == 'k' || buf[0] == 'l' || buf[0] == 'm') {
        player_id = 1;
      } else {
        continue;
      }

      // envoyer touche au serveur
      struct client_input input = {
          .id = player_id,
          .input = convert_key_to_movement(buf[0]),
      };
      input.id = htonl(input.id);
      printf("sending input %d to server, player %d, byte_count: %d\n",
             input.input, input.id, byte_count);
      CHECK(send(socket_fd, &input, sizeof(struct client_input), 0));
    }
  }

  CHECK(close(socket_fd));
  return 0;
}
