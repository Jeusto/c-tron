#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

// arguments temporairement hardcodés
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define SERVER_PORT 5555
#define REFRESH_RATE 100

display_info init_board() {}

void send_display_info_to_client() {}

void update_board() {}

void game_loop() {}

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
  int clients[MAX_CLIENTS] = {0};
  int current_nb_clients = 0;
  int max_fd, new_socket, activity;
  struct sockaddr_in6 client_addr;
  int addr_size = sizeof client_addr;
  int master_socket;

  master_socket = socket_factory();

  CHECK(listen(master_socket, MAX_CLIENTS));

  FD_ZERO(&readfds);
  FD_SET(master_socket, &readfds);
  max_fd = master_socket;

  // boucle pour attendre connexions ou messages des clients
  while (1) {
    printf("Waiting for connections or messages...\n");
    CHECK(activity = select(max_fd + 1, &readfds, NULL, NULL, NULL));

    // si evenement sur le socket principal, c'est une nouvelle connexion
    if (FD_ISSET(master_socket, &readfds)) {
      CHECK(new_socket = accept(master_socket, (struct sockaddr *)&client_addr,
                                (socklen_t *)&addr_size));
      if (current_nb_clients >= MAX_CLIENTS) {
        printf("New connection denied because max clients reached\n");
        close(new_socket);
      }

      else {
        printf("New connection\n");

        // recevoir nombre de joueurs sur ce client
        uint32_t some_long = 0;
        uint32_t network_byte_order;
        CHECK(read(new_socket, &network_byte_order, sizeof(uint32_t)));

        current_nb_clients += ntohl(network_byte_order);
        printf("Nombre de joueurs %d\n", current_nb_clients);

        for (int i = 0; i < MAX_CLIENTS; i++) {
          if (clients[i] == 0) {
            if (clients[i] == 0) {
              printf("Adding to list of client sockets as %d\n", i);
              clients[i] = new_socket;
              FD_SET(new_socket, &readfds);
              break;
            }
          }
        }
      }

      // parcourir les clients pour voir s'il y a des messages
      for (int i = 0; i < MAX_CLIENTS; i++) {
        int client_sd = clients[i];
        int valread = 0;
        if (FD_ISSET(client_sd, &readfds)) {
          char buffer[BUF_SIZE];
          CHECK(valread = read(client_sd, buffer, BUF_SIZE));
          buffer[valread] = '\0';

          if (valread == 0) {
            // deconnexion
            printf("Client with socket %d deconnected\n", client_sd);
            // supprimer le socket de la liste
            close(client_sd);
            clients[i] = 0;
          } else {
            // message recu
            printf("Message from client %d of size %ld: %s \n", i,
                   strlen(buffer), buffer);

            // envoyer a tout le monde sauf l'emetteur
            for (int i = 0; i < MAX_CLIENTS; i++) {
              if (clients[i] != 0 && clients[i] != client_sd) {
                CHECK(send(clients[i], buffer, strlen(buffer), 0));
              }
            }
          }
        }
      }
    }

    CHECK(close(master_socket));
    return 0;
  }
}