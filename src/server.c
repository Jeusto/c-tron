#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "../include/common.h"

// arguments temporairement hardcodés
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define SERVER_PORT 5555
#define REFRESH_RATE 100

#define EMPTY -1

typedef struct player {
  int id;
  int x;
  int y;
  int trail_up;
  int direction;
  int alive;
} player;

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

void initGame(display_info *board, int maxX, int maxY) {
  int x, y;
  for (x = 0; x < maxX; x++) {
    for (y = 0; y < maxY; y++) {
      if (x == 0 || x == maxX - 1 || y == 0 || y == maxY - 1) {
        board->board[x][y] = WALL;
      } else {
        board->board[x][y] = EMPTY;
      }
    }
  }
  board->winner = -1;
}

// ajoute un joueur
player add_player(display_info *board, int id_player) {
  int maxX = XMAX;
  int maxY = YMAX;
  player new_player;
  switch (id_player) {
    case 0:
      board->board[1][1] = id_player;
      new_player.x = 1;
      new_player.y = 1;
      new_player.direction = DOWN;
      break;
    case 1:
      board->board[maxX - 1][maxY - 1] = id_player;
      new_player.x = maxX - 1;
      new_player.y = maxY - 1;
      new_player.direction = UP;
      break;
      /*case 2:
        board.board[maxX/2][1] = id_player;
        break;
      case 3:
        board.board[1][maxY/2] = id_player;
        break;*/
  }
  new_player.id = id_player;
  // trail actif par defaut
  new_player.trail_up = 1;
  new_player.alive = 1;
  return new_player;
}

// supprime un joueur
void remove_player(display_info *board, player *player) {
  board->board[player->x][player->y] = EMPTY;
  player->alive = 0;
}

void restart(display_info *board, int maxX, int maxY, player *players,
             int nbPlayers) {
  initGame(board, maxX, maxY);
  int i;
  for (i = 0; i < nbPlayers; i++) {
    players[i] = add_player(board, players[i].id);
  }
}

int checkWinner(player *players) {
  if (players[0].alive == 0 && players[1].alive == 0) {
    return TIE;
  }
  if (players[0].alive == 0) {
    return players[1].id;
  }
  if (players[1].alive == 0) {
    return players[0].id;
  }
  return -1;
}

// deplace un joueur
void move_player(display_info *board, player *p, int direction) {
  int x = p->x;
  int y = p->y;
  int id = p->id;
  switch (direction) {
    case UP:
      p->y - 1;
      break;
    case DOWN:
      p->y + 1;
      break;
    case LEFT:
      p->x - 1;
      break;
    case RIGHT:
      p->x + 1;
      break;
    case TRAIL_UP:
      p->trail_up = p->trail_up == 0 ? 1 : 0;
      // p->trail_up += 1%2;
      break;
  }

  // A VERIFIER SI CA POSE PROBLEME CAR UPDATE DU TRAIL AVANT LE DEPLACEMENT DU
  // JOUEUR mais normalement ca devrait pas poser de probleme
  if (p->trail_up == 1) {
    board->board[x][y] = 50 + id;
  } else {
    board->board[x][y] = EMPTY;
  }
}

void check_collision(display_info *board, player *p) {
  if (board->board[p->x][p->y] != EMPTY) {
    p->alive = 0;
  }
}

void update_board(display_info *board, player *p) {
  int x = p->x;
  int y = p->y;
  int id = p->id;
  board->board[x][y] = id;
}

/*
1 : regarde si la partie est finie
2 : update le board
*/

void update_game(display_info *board, player *players, int nbr_players) {
  int i;
  for (i = 0; i < nbr_players; i++) {
    check_collision(board, &players[i]);
  }
  board->winner = checkWinner(players);
  if (board->winner != -1) {
    remove_player(board, &players[(board->winner + 1) % 2]);
    return;
  }
  for (i = 0; i < nbr_players; i++) {
    update_board(board, &players[i]);
  }
}

void send_board_to_all_clients(display_info *board, int sockets[],
                               int nbr_sockets) {
  for (int i = 0; i < nbr_sockets; i++) {
    if (sockets[i] != 0) {
      CHECK(send(sockets[i], board, sizeof(display_info), 0));
    }
  }
}

int main(int argc, char *argv[]) {
  fd_set readfds;
  int list_sockets[MAX_CLIENTS] = {0};
  int nb_joueurs_sur_ce_client = 0;
  int nb_joueurs = 0;
  int max_fd, new_socket, activity;
  struct sockaddr_in6 client_addr;
  int addr_size = sizeof client_addr;
  int master_socket;
  int refresh_rate = REFRESH_RATE;
  int running = 0;
  int byte_count = 0;
  // temporaire
  (void)argv;
  (void)argc;

  master_socket = socket_factory();

  CHECK(listen(master_socket, MAX_CLIENTS));

  FD_ZERO(&readfds);
  FD_SET(master_socket, &readfds);
  max_fd = master_socket;

  // initialisation du board
  display_info board[XMAX][YMAX];
  initGame(board, XMAX, YMAX);
  player list_players[MAX_CLIENTS];

  // boucle pour attendre connexions ou messages des list_sockets
  printf("Waiting for connections or messages...\n");

  while (1) {
    // select with refresh rate
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = refresh_rate * 1000;
    CHECK(activity = select(max_fd + 1, &readfds, NULL, NULL, &tv));

    printf("%d\n", activity);

    // si evenement sur le socket principal, c'est une nouvelle connexion
    if (FD_ISSET(master_socket, &readfds)) {
      printf("nouvelle co\n");
      CHECK(new_socket = accept(master_socket, (struct sockaddr *)&client_addr,
                                (socklen_t *)&addr_size));
      if (nb_joueurs + nb_joueurs_sur_ce_client > MAX_CLIENTS || running == 1) {
        printf("New connection denied because max list_sockets reached\n");
        close(new_socket);
      }

      else {
        printf("New connection\n");

        // recevoir nombre de joueurs sur ce client
        uint32_t some_long = 0;
        uint32_t network_byte_order;

        struct client_init_infos init_info;
        CHECK(byte_count = recv(new_socket, &init_info,
                                sizeof(struct client_init_infos), 0));

        nb_joueurs_sur_ce_client = init_info.nb_players;

        if (nb_joueurs_sur_ce_client == 1) {
          list_players[nb_joueurs] = add_player(board, nb_joueurs);
          nb_joueurs++;
        } else if (nb_joueurs_sur_ce_client == 2) {
          list_players[nb_joueurs] = add_player(board, nb_joueurs);
          nb_joueurs++;
          list_players[nb_joueurs] = add_player(board, nb_joueurs);
          nb_joueurs++;
        }

        printf("Adding to list of client sockets as\n");
        list_sockets[new_socket] = new_socket;
        FD_SET(new_socket, &readfds);

        if (nb_joueurs == MAX_CLIENTS) {
          running = 1;
        }
      }
    }

    // evenement entree standard (verifier si quit ou restart)
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
      char buf[BUF_SIZE];
      fgets(buf, BUF_SIZE, stdin);

      if (strcmp(buf, "quit") == 0) {
        //
      } else if (strcmp(buf, "restart") == 0) {
        //
      }
    }

    // parcourir les list_sockets pour voir s'il y a des messages
    if (running) {
      printf("parcourir list_sockets\n");
      for (int i = 0; i < MAX_CLIENTS; i++) {
        int client_sd = list_sockets[i];
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
            // remove_player(board, &list_players);
            nb_joueurs--;
            list_sockets[i] = 0;
          } else {
            // message recu
            printf("Message from client %d of size %ld: %s \n", i,
                   strlen(buffer), buffer);

            struct client_input input;
            CHECK(byte_count =
                      recv(client_sd, &input, sizeof(struct client_input), 0));

            move_player(board, &list_players[input.id], input.input);
          }
        }
      }
    }

    if (activity == 0 && running) {
      printf("timeout\n");
      // timeout
      update_game(board, list_players, nb_joueurs);
      send_board_to_all_clients(board, list_sockets, MAX_CLIENTS);
    }
  }

  CHECK(close(master_socket));
  return 0;
}