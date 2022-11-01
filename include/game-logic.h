// TODO: separer declarations dans.h et implementations dans.c puis modifier le
// makefile
#include <pthread.h>

#include "../include/common.h"

// arguments temporairement hardcodés
#define BUF_SIZE 1024
#define MAX_JOUEURS 2
#define SERVER_PORT 5555
#define REFRESH_RATE 1000

typedef struct player {
  int id;
  int x;
  int y;
  int trail_up;
  int direction;
  int alive;
} player;

typedef struct thread_arg {
  int socket;
  int *sockets;
  int *nbr_sockets;
  player *players;
  int *nbr_players;
  display_info *board;
} thread_arg;

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
      board->board[maxX - 2][maxY - 2] = id_player;
      new_player.x = maxX - 2;
      new_player.y = maxY - 2;
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

  // print board
  for (int x = 0; x < maxX; x++) {
    for (int y = 0; y < maxY; y++) {
      printf("%d ", board->board[x][y]);
    }
    printf("\n");
  }
  printf("\n");

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

void *fonction_thread_jeu(void *arg) {
  thread_arg *t_arg = (thread_arg *)arg;
  printf("thread jeu\n");

  while (1) {
    // update game
    printf("update game\n");
    // if (game_running) {
    update_game(t_arg->board, t_arg->players, *t_arg->nbr_players);
    send_board_to_all_clients(t_arg->board, t_arg->sockets,
                              *t_arg->nbr_sockets);
    // }
    usleep(REFRESH_RATE * 1000);
  }
}