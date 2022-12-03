#include <errno.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "../include/game-logic.h"

/// @brief Cree un socket, le lie a l'adresse et le port
/// @param server_port Port du serveur
/// @return Socket cree
int create_socket(int server_port) {
  SAI server_addr;
  int master_socket, opt = 1;

  // Creer et configurer socket principal
  CHECK(master_socket = socket(AF_INET, SOCK_STREAM, 0));
  CHECK(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)));

  // Preparer adresse serveur
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // Associer socket a l'adresse
  CHECK(bind(master_socket, (struct sockaddr *)&server_addr,
             sizeof(server_addr)));

  return master_socket;
}

int main(int argc, char *argv[]) {
  // Verifier les arguments
  if (argc != 3) {
    printf("Usage: %s [port_serveur] [refresh_rate]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Initialiser les variables liees au jeu
  int server_port = atoi(argv[1]);
  int refresh_rate = atoi(argv[2]);
  int max_players = MAX_PLAYERS;
  int player_count = 0, game_running = 0;
  player players_list[max_players];
  display_info game_info;
  initial_player_position init[4] = {{2, YMAX / 2, RIGHT},
                                     {XMAX - 3, YMAX / 2, LEFT},
                                     {XMAX / 2, 2, DOWN},
                                     {XMAX / 2, YMAX - 2, UP}};

  // Initialiser les variables liees au serveur
  int new_socket, activity, bytes_received = 0;
  int client_count = 0, players_on_this_client = 0;
  int sockets_list[max_players];
  for (int i = 0; i < max_players; i++) {
    sockets_list[i] = 0;
  }

  // Creer et configurer socket principal
  int master_socket = create_socket(server_port);
  CHECK(listen(master_socket, max_players));

  // Preparer select
  fd_set master_fds, read_fds;
  SAI client_addr;
  int addr_size = sizeof client_addr;
  int max_fd = master_socket;

  FD_ZERO(&read_fds);
  FD_ZERO(&master_fds);
  FD_SET(STDIN_FILENO, &master_fds);
  FD_SET(master_socket, &master_fds);

  TV tv;
  gettimeofday(&tv, NULL);
  long int temps_precedent_refresh = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

  // Boucle pour attendre des messages, des commandes et lancer le jeu
  while (1) {
    // Attendre une activite
    read_fds = master_fds;
    TV tv_select = {0, 1000 * refresh_rate};
    CHECK(activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv_select));

    // Evenement sur le socket principal = nouvelle tentative de connexion
    if (FD_ISSET(master_socket, &read_fds)) {
      CHECK(new_socket = accept(master_socket, (SA *)&client_addr,
                                (socklen_t *)&addr_size));

      // Recevoir le nombre de joueurs sur ce client
      struct client_init_infos init_info;
      CHECK(bytes_received = recv(new_socket, &init_info,
                                  sizeof(struct client_init_infos), 0));
      players_on_this_client = init_info.nb_players;

      // Refuser s'il y aura trop de joueurs dans la partie
      if (player_count + players_on_this_client > max_players) {
        close(new_socket);
      }

      // Sinon, accepter
      else {
        for (int i = 0; i < players_on_this_client; i++) {
          add_player(players_list, &game_info, init, player_count, new_socket,
                     i);
          player_count++;
        }

        sockets_list[client_count++] = new_socket;
        FD_SET(new_socket, &master_fds);
        if (new_socket > max_fd) max_fd = new_socket;

        // Si le nombre de joueurs est suffisant, lancer le jeu
        if (player_count == max_players) {
          restart_game(&game_info, init, players_list, &game_running,
                       max_players);
        }
      }
    }

    // Evenement sur stdin = commande pour le serveur (restart ou quit)
    else if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      char buf[BUF_SIZE];
      fgets(buf, BUF_SIZE, stdin);
      buf[strlen(buf) - 1] = '\0';

      if (strcmp(buf, "quit") == 0) {
        game_running = 0;
        break;
      } else if (strcmp(buf, "restart") == 0) {
        restart_game(&game_info, init, players_list, &game_running,
                     max_players);
      }
    }

    // Mettre a jour le jeu si le jeu est en cours et il s'est ecoule
    // suffisamment de temps depuis le dernier refresh rate
    if (game_running) {
      TV tv;
      gettimeofday(&tv, NULL);
      long int temps_actuel_en_ms = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

      if ((temps_actuel_en_ms - temps_precedent_refresh) > refresh_rate) {
        temps_precedent_refresh = temps_actuel_en_ms;
        update_game(&game_info, players_list, player_count, &game_running);
        send_board_to_all_clients(&game_info, sockets_list, client_count);
      }
    }

    // Parcourir tout les sockets pour voir s'il y a des messages a recevoir
    for (int i = 0; i < client_count; i++) {
      int client_sd = sockets_list[i];
      int bytes_received = 0;

      if (FD_ISSET(client_sd, &read_fds)) {
        // Recevoir message d'un client
        struct client_input input;
        CHECK(bytes_received =
                  recv(client_sd, &input, sizeof(struct client_input), 0));
        input.id = input.id;

        // 0 bytes recu = client deconnecte
        if (bytes_received == 0) {
          // Fermer le serveur
          printf("Un client a quitte le jeu, le serveur va se deconnecter.\n");
          close(master_socket);
          exit(EXIT_SUCCESS);
        }

        // Jeu en cours & message recu = mettre a jour la direction d'un joueur
        else if (game_running) {
          update_player_direction(players_list, player_count, client_sd,
                                  input.id, input.input);
        }
      }
    }
  }

  CHECK(close(master_socket));
  CHECK(close(new_socket));
  return 0;
}