#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// TODO: ne pas utiliser de variable globale
int game_running = 0;

#include "../include/common.h"
#include "../include/game-logic.h"

int socket_factory() {
  int master_socket;
  struct sockaddr_in6 server_addr;
  int opt = 1;

  // creer socket principale
  CHECK(master_socket = socket(AF_INET6, SOCK_STREAM, 0));

  // accepter plusieurs connexions
  CHECK(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)));

  // preparer adresse serveur
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(SERVER_PORT);
  server_addr.sin6_addr = in6addr_any;

  // associer socket a adresse
  CHECK(bind(master_socket, (struct sockaddr *)&server_addr,
             sizeof(server_addr)));

  return master_socket;
}

int main(int argc, char *argv[]) {
  fd_set readfds;
  int list_sockets[MAX_JOUEURS] = {0};
  int nb_joueurs_sur_ce_client = 0;
  int nb_joueurs_actuel = 0;
  int nb_clients_actuel = 0;
  int max_fd, new_socket, activity;
  struct sockaddr_in6 client_addr;
  int addr_size = sizeof client_addr;
  int master_socket;
  int refresh_rate = REFRESH_RATE;
  int byte_count = 0;
  (void)argv;
  (void)argc;

  master_socket = socket_factory();
  CHECK(listen(master_socket, MAX_JOUEURS));
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);
  FD_SET(master_socket, &readfds);
  max_fd = master_socket;

  // initialisation du board
  display_info board[XMAX][YMAX];
  initGame(board, XMAX, YMAX);
  player list_players[MAX_JOUEURS];

  printf("Waiting for connections or messages...\n");

  while (1) {
    CHECK(activity = select(max_fd + 1, &readfds, NULL, NULL, NULL));

    // si evenement sur le socket principal, c'est une nouvelle connexion
    if (FD_ISSET(master_socket, &readfds)) {
      printf("Nouvelle connexion\n");
      CHECK(new_socket = accept(max_fd, (struct sockaddr *)&client_addr,
                                (socklen_t *)&addr_size));

      // recevoir nombre de joueurs sur ce client
      uint32_t some_long = 0;
      uint32_t network_byte_order;

      struct client_init_infos init_info;
      CHECK(byte_count = recv(new_socket, &init_info,
                              sizeof(struct client_init_infos), 0));

      nb_joueurs_sur_ce_client = ntohl(init_info.nb_players);
      printf("nb_joueurs_sur_ce_client : %d\n", nb_joueurs_sur_ce_client);

      // refuser si trop de joueurs
      if (nb_joueurs_actuel + nb_joueurs_sur_ce_client > MAX_JOUEURS) {
        printf("New connection denied because max list_sockets reached\n");
        close(new_socket);
      }

      // sinon, accepter
      else {
        for (int i = 0; i < nb_joueurs_sur_ce_client; i++) {
          list_players[nb_joueurs_actuel++] =
              add_player(board, nb_joueurs_actuel);
        }

        printf("Adding to list of client sockets\n");
        printf("socket %d\n", new_socket);
        list_sockets[nb_clients_actuel] = new_socket;
        nb_clients_actuel++;
        FD_SET(new_socket, &readfds);
        if (new_socket > max_fd) max_fd = new_socket;

        // si max joueurs atteint, lancer le jeu
        if (nb_joueurs_actuel == MAX_JOUEURS) {
          game_running = 1;

          // lancement du thread de jeu
          pthread_t thread_jeu;
          thread_arg t_arg;
          t_arg.socket = new_socket;
          t_arg.sockets = list_sockets;
          t_arg.nbr_sockets = &nb_joueurs_actuel;
          t_arg.players = list_players;
          t_arg.nbr_players = &nb_joueurs_actuel;
          t_arg.board = board;
          pthread_create(&thread_jeu, NULL, fonction_thread_jeu, &t_arg);
        }
      }

      continue;
    }

    // TODO: evenement entree standard (verifier si quit ou restart)
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
      char buf[BUF_SIZE];
      fgets(buf, BUF_SIZE, stdin);
      printf("buf = %s", buf);
      if (strcmp(buf, "quit") == 0) {
        //
      } else if (strcmp(buf, "restart") == 0) {
        //
      }
    }

    // parcourir les list_sockets pour voir s'il y a des messages
    if (game_running) {
      printf("Parcourir list_sockets\n");

      for (int i = 0; i < nb_clients_actuel; i++) {
        printf("client = %d, socket = %d\n", i, list_sockets[i]);
        int client_sd = list_sockets[i];
        int byte_count = 0;
        if (FD_ISSET(client_sd, &readfds)) {
          struct client_input input;
          // TODO: recv peut faire lecture partielle donc faire une boucle ou
          // utiliser MSG_WAITALL ?
          CHECK(byte_count =
                    recv(client_sd, &input, sizeof(struct client_input), 0));
          input.id = ntohl(input.id);
          if (byte_count == 0) {
            // deconnexion
            printf("Client with socket %d deconnected\n", client_sd);
            // supprimer le socket de la liste
            close(client_sd);
            // remove_player(board, &list_players);
            nb_joueurs_actuel--;  // TODO: verifier si 1 ou 2 joueurs ont deco
            nb_clients_actuel--;
            FD_CLR(client_sd, &readfds);
            list_sockets[i] = 0;
            max_fd--;
          } else {
            // message recu
            printf("joueur %d : movement %d\n", input.id, input.input);
            update_player_direction(board, &list_players[input.id],
                                    input.input);
          }
        }
      }
    }
  }

  CHECK(close(master_socket));
  return 0;
}