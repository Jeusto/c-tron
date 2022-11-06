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

int create_socket() {
  struct sockaddr_in server_addr;
  int master_socket, opt = 1;

  // Creer et configurer socket principal
  CHECK(master_socket = socket(AF_INET, SOCK_STREAM, 0));
  CHECK(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)));

  // Preparer adresse serveur
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // Associer socket a l'adresse
  CHECK(bind(master_socket, (struct sockaddr *)&server_addr,
             sizeof(server_addr)));

  return master_socket;
}

int main(int argc, char *argv[]) {
  int nb_joueurs = 0, nb_clients = 0, nb_joueurs_sur_ce_client = 0;
  int max_fd, master_socket, new_socket, activity;
  int list_sockets[MAX_JOUEURS] = {0};
  player list_joueurs[MAX_JOUEURS];
  int bytes_received = 0;
  fd_set master_fds, read_fds;
  struct sockaddr_in client_addr;
  int addr_size = sizeof client_addr;
  display_info game_info;

  // Verifier le nombre d'arguments
  // if (argc != 3) {
  //   printf("Usage: %s [port_serveur] [refresh_rate] \n", argv[0]);
  //   exit(EXIT_FAILURE);
  // }

  // Creer et configurer socket principal
  master_socket = create_socket();
  CHECK(listen(master_socket, MAX_JOUEURS));

  // Preparer select
  FD_ZERO(&read_fds);
  FD_ZERO(&master_fds);
  FD_SET(STDIN_FILENO, &master_fds);
  FD_SET(master_socket, &master_fds);
  max_fd = master_socket;

  // Iitialiser le jeu
  init_game(&game_info, XMAX, YMAX);

  // Boucle pour attendre des messages, des commandes et lancer le jeu
  printf("Waiting for connections or messages...\n");
  while (1) {
    // Attendre une activite
    read_fds = master_fds;
    CHECK(activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL));

    // Evenement sur le socket principal = nouvelle tentative de connexion
    if (FD_ISSET(master_socket, &read_fds)) {
      printf("Nouvelle connexion\n");
      CHECK(new_socket = accept(master_socket, (struct sockaddr *)&client_addr,
                                (socklen_t *)&addr_size));

      // Recevoir le nombre de joueurs sur ce client
      struct client_init_infos init_info;
      CHECK(bytes_received = recv(new_socket, &init_info,
                                  sizeof(struct client_init_infos), 0));

      nb_joueurs_sur_ce_client = ntohl(init_info.nb_players);
      printf("nb_joueurs_sur_ce_client : %d\n", nb_joueurs_sur_ce_client);

      // Refuser si trop de joueurs
      if (nb_joueurs + nb_joueurs_sur_ce_client > MAX_JOUEURS ||
          nb_joueurs_sur_ce_client > 2) {
        printf("New connection denied\n");
        close(new_socket);
      }

      // sinon, accepter
      else {
        for (int i = 0; i < nb_joueurs_sur_ce_client; i++) {
          list_joueurs[nb_joueurs++] =
              add_player(&game_info, nb_joueurs, new_socket, i);
        }

        printf("Adding to list of client sockets\n");
        printf("socket %d\n", new_socket);
        list_sockets[nb_clients++] = new_socket;
        printf("nb_clients_actuel : %d\n", nb_clients);
        FD_SET(new_socket, &master_fds);
        if (new_socket > max_fd) max_fd = new_socket;

        // si max joueurs atteint, lancer le jeu
        if (nb_joueurs == MAX_JOUEURS) {
          game_running = 1;

          // lancement du thread de jeu
          pthread_t thread_jeu;
          thread_arg t_arg;
          // TODO: gerer section critique, mutex ou pas ?
          t_arg.socket = master_socket;
          t_arg.sockets = list_sockets;
          t_arg.nbr_sockets = &nb_clients;
          t_arg.joueurs = list_joueurs;
          t_arg.nbr_players = &nb_joueurs;
          t_arg.game_info = &game_info;
          t_arg.game_running = &game_running;
          pthread_create(&thread_jeu, NULL, fonction_thread_jeu, &t_arg);
        }
      }
    }

    // TODO: evenement entree standard (verifier si quit ou restart)
    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      char buf[BUF_SIZE];
      fgets(buf, BUF_SIZE, stdin);
      buf[strlen(buf) - 1] = '\0';
      if (strcmp(buf, "quit") == 0) {
        printf("Quitting...\n");
        exit(0);
        game_running = 0;
      } else if (strcmp(buf, "restart") == 0) {
        // FIXME: erreur stack smashing detected
        // restart(&game_info, XMAX, YMAX, list_joueurs, nb_joueurs);
      }
    }

    // parcourir les list_sockets pour voir s'il y a des messages
    if (game_running) {
      printf("Parcourir list_sockets\n");

      for (int i = 0; i < nb_clients; i++) {
        printf("joueur = %d, socket = %d\n", i, list_sockets[i]);

        int client_sd = list_sockets[i];
        int bytes_received = 0;
        if (FD_ISSET(client_sd, &read_fds)) {
          struct client_input input;
          // FIXME: recv peut faire lecture partielle donc faire une boucle ou
          // utiliser MSG_WAITALL ?
          CHECK(bytes_received =
                    recv(client_sd, &input, sizeof(struct client_input), 0));
          input.id = ntohl(input.id);
          printf("Recu un input du socket %d, joueur id = %d\n", client_sd,
                 input.id);

          if (bytes_received == 0) {
            // deconnexion
            printf("Client with socket %d deconnected\n", client_sd);
            // supprimer le socket de la liste
            close(client_sd);
            // kill_player(board, &list_joueurs);
            nb_joueurs--;  // FIXME: verifier si 1 ou 2 joueurs ont deco
            nb_clients--;
            FD_CLR(client_sd, &master_fds);
            list_sockets[i] = 0;
            max_fd--;
          } else {
            // message recu
            for (int j = 0; j < nb_joueurs; j++) {
              if (list_joueurs[j].socket_associe == client_sd &&
                  list_joueurs[j].id_sur_socket == input.id) {
                printf("Joueur numero %d, sur socket %d, id sur socket = %d\n",
                       j, client_sd, input.id);
                update_player_direction(&game_info, &list_joueurs[j],
                                        input.input);
              }
            }
          }
        }
      }
    }
  }

  CHECK(close(master_socket));
  return 0;
}