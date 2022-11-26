#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "client-display.c"
#include "common.h"

int get_player_corresponding_to_key(char key) {
  int player_id = -1;

  switch (key) {
    case KEY_UP_P1:
    case KEY_DOWN_P1:
    case KEY_LEFT_P1:
    case KEY_RIGHT_P1:
    case KEY_TRAIL_P1:
      player_id = 0;
      break;

    case KEY_UP_P2:
    case KEY_DOWN_P2:
    case KEY_LEFT_P2:
    case KEY_RIGHT_P2:
    case KEY_TRAIL_P2:
      player_id = 1;
      break;
  }

  return player_id;
}

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
  int socket_fd, max_fd, activity = 0, bytes_received = 0;
  struct sockaddr_in server_addr;
  fd_set read_fds;

  // Verifier le nombre d'arguments
  if (argc != 4) {
    debug("Usage: %s [IP_serveur] [port_serveur] [nb_joueurs] \n", argv[0]);
    exit(EXIT_FAILURE);
  }
  // Initialiser l'affichage
  tune_terminal();
  init_graphics();
  srand(time(NULL));

  // Creer socket
  CHECK(socket_fd = socket(AF_INET, SOCK_STREAM, 0));
  max_fd = socket_fd;

  // Preparer adresse serveur
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(atoi(argv[2]));
  inet_aton(argv[1], &(server_addr.sin_addr));

  // Se connecter au serveur
  CHECK(
      connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)));

  // Envoyer nombre de joueurs sur ce client au serveur
  struct client_init_infos init_info = {
      .nb_players = atoi(argv[3]),
  };
  CHECK(send(socket_fd, &init_info, sizeof(struct client_init_infos), 0));

  // Boucle pour attendre des messages du serveur et afficher le jeu
  while (1) {
    // Preparer select
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(socket_fd, &read_fds);

    // Attendre une activite
    CHECK(activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL));

    // Message recu du serveur
    if (FD_ISSET(socket_fd, &read_fds)) {
      display_info game_info;
      CHECK(bytes_received =
                recv(socket_fd, &game_info, sizeof(display_info), 0));

      // 0 bytes recu = serveur deconnecte
      if (bytes_received == 0) {
        debug("server closed connection\n");
        break;
      }

      // check if correctly received
      if (bytes_received != sizeof(display_info)) {
        debug("received %d bytes instead of %d\n", bytes_received,
              sizeof(display_info));
        break;
      }

      // Message recu = mettre a jour l'affichage
      else {
        debug("winner: %d\n", game_info.winner);
        update_display(&game_info);
      }
    }

    // Un des joueurs a appuye sur une touche
    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      char buf[BUF_SIZE];
      CHECK(bytes_received = read(STDIN_FILENO, buf, BUF_SIZE));
      buf[bytes_received] = '\0';

      struct client_input client_input = {
          .id = get_player_corresponding_to_key(buf[0]),
          .input = convert_key_to_movement(buf[0]),
      };

      // Envoyer touche au serveur si c'est valide
      if (client_input.id != -1 && client_input.input != -1) {
        CHECK(send(socket_fd, &client_input, sizeof(struct client_input), 0));
      }
    }
  }

  CHECK(close(socket_fd));
  return 0;
}
