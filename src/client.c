#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>

#include "../include/display.h"

/// @brief Renvoie le joueur correspondant a une touche donnee
/// @param key Touche appuyee
/// @return Joueur correspondant a la touche
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

/// @brief Renvoie la direction correspondant a une touche donnee
/// @param c Touche appuyee
/// @return Direction correspondant a la touche
int convert_key_to_direction(char key) {
  int input = -1;

  switch (key) {
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
  SAI server_addr;
  fd_set read_fds;

  // Verifier les arguments
  if (argc != 4) {
    printf("Usage: %s [IP_serveur] [port_serveur] [nb_joueurs] \n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int player_count = atoi(argv[3]);
  if (player_count < 1 || player_count > 2) {
    printf("Nombre de joueurs doit etre 1 ou 2\n");
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
  CHECK(connect(socket_fd, (SA*)&server_addr, sizeof(server_addr)));

  // Envoyer nombre de joueurs sur ce client au serveur
  struct client_init_infos init_info = {
      // Il faudrait potentiellement utiliser htonl() mais l'executable du prof
      // n'utilise pas ntohl donc on ne l'a pas fait
      .nb_players = atoi(argv[3]),
  };
  CHECK(send(socket_fd, &init_info, sizeof(struct client_init_infos), 0));

  // Boucle pour attendre des messages du serveur et afficher le jeu
  show_centered_message("En attente des autres joueurs...");
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
        show_centered_message(
            "Le serveur a ete deconnecte. Merci d'avoir joue!");
        break;
      }

      // Message recu > 0 bytes = mettre a jour l'affichage
      else {
        update_display(&game_info);
      }
    }

    // Un des joueurs a appuye sur une touche
    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      char key = getchar();

      // Si la touche est la touche pour quitter, quitter le jeu
      if (key == KEY_QUIT) {
        show_centered_message("Merci d'avoir joue!");
        break;
      }

      // Sinon creer la structure a envoyer au serveur
      struct client_input client_input = {
          .id = get_player_corresponding_to_key(key),
          .input = convert_key_to_direction(key),
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
