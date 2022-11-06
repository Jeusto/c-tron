// TODO: separer declarations dans.h et implementations dans.c puis modifier le
// makefile
#include <pthread.h>
#include <unistd.h>

#include "../include/common.h"

// arguments temporairement hardcodés
#define BUF_SIZE 1024
#define MAX_JOUEURS 2
#define SERVER_PORT 5555
#define REFRESH_RATE 300

typedef struct player {
  int id;
  int x;
  int y;
  int trail_up;
  int direction;
  int isAlive;
  int socket_associe;
  int id_sur_socket;
} player;

typedef struct thread_arg {
  int socket;
  int *sockets;
  int *nbr_sockets;
  player *joueurs;
  int *nbr_players;
  display_info *game_info;
  int *game_running;
} thread_arg;

void init_game(display_info *game_info, int maxX, int maxY) {
  int x, y;
  for (x = 0; x < maxX; x++) {
    for (y = 0; y < maxY; y++) {
      if (x == 0 || x == maxX - 1 || y == 0 || y == maxY - 1) {
        game_info->board[x][y] = WALL;
      } else {
        game_info->board[x][y] = EMPTY;
      }
    }
  }
  game_info->winner = -1;
}

player add_player(display_info *game_info, int id_player, int new_socket,
                  int id_sur_socket) {
  int maxX = XMAX;
  int maxY = YMAX;
  player new_player;

  switch (id_player) {
    case 0:
      game_info->board[1][1] = id_player;
      new_player.x = 1;
      new_player.y = 1;
      new_player.direction = DOWN;
      break;
    case 1:
      game_info->board[maxX - 2][maxY - 2] = id_player;
      new_player.x = maxX - 2;
      new_player.y = maxY - 2;
      new_player.direction = UP;
      break;
      // case 2:
      //   game_info->board[maxX / 2][1] = id_player;
      //   break;
      // case 3:
      //   game_info->board[1][maxY / 2] = id_player;
      //   break;
  }
  new_player.id = id_player;
  // trail actif par defaut
  new_player.trail_up = 1;
  new_player.isAlive = 1;
  new_player.socket_associe = new_socket;
  new_player.id_sur_socket = id_sur_socket;

  return new_player;
}

// supprime un joueur
void kill_player(display_info *game_info, player *player) {
  player->isAlive = 0;
}

void restart(display_info *game_info, int maxX, int maxY, player *joueurs,
             int nbPlayers) {
  init_game(game_info, maxX, maxY);
  int i;
  for (i = 0; i < nbPlayers; i++) {
    joueurs[i] =
        add_player(&game_info, joueurs[i].id, joueurs[i].socket_associe,
                   joueurs[i].id_sur_socket);
  }
}

int check_winner(display_info *game_info, player *joueurs, int nbr_players) {
  // si un joueur est mort, on verifie qu'un autre ne serait pas mort au meme
  // moment
  if (!joueurs[0].isAlive || !joueurs[1].isAlive) {
    int i;
    for (i = 0; i < nbr_players; i++) {
      if (joueurs[i].isAlive) {
        update_player_position(game_info, &joueurs[i]);
      }
    }
  }

  if (joueurs[0].isAlive == 0 && joueurs[1].isAlive == 0) {
    return TIE;
  }
  if (joueurs[0].isAlive == 0) {
    return joueurs[1].id;
  }
  if (joueurs[1].isAlive == 0) {
    return joueurs[0].id;
  }

  return -1;
}

void update_player_direction(display_info *game_info, player *p,
                             int direction) {
  // on autorise pas de revenir en arriere, c'est a dire si le joueur va a
  // gauche il peut pas directement aller a droite comme dans le snake classique
  switch (direction) {
    case UP:
      if (p->direction != DOWN) {
        p->direction = UP;
      }
      break;
    case DOWN:
      if (p->direction != UP) {
        p->direction = DOWN;
      }
      break;
    case LEFT:
      if (p->direction != RIGHT) {
        p->direction = LEFT;
      }
      break;
    case RIGHT:
      if (p->direction != LEFT) {
        p->direction = RIGHT;
      }
      break;
    case TRAIL_UP:
      p->trail_up = !p->trail_up;
      break;
  }
}

void check_collision(display_info *game_info, player *p, int x, int y) {
  // check collission
  if (game_info->board[x][y] != EMPTY) {
    printf("🚨 player %d collided\n", p->id);
    kill_player(game_info, p);
  }
}

void update_player_position(display_info *game_info, player *p) {
  int x = p->x;
  int y = p->y;
  int id = p->id;

  // remove all trail if trail_up == 0
  if (p->trail_up == 0) {
    int i, j;
    for (i = 0; i < XMAX; i++) {
      for (j = 0; j < YMAX; j++) {
        if (game_info->board[i][j] == 50 + id) {
          game_info->board[i][j] = EMPTY;
        }
      }
    }
  }

  switch (p->direction) {
    case UP:
      check_collision(game_info, p, x, y - 1);
      if (p->isAlive == 1) {
        game_info->board[x][y - 1] = id;
        p->y--;
      }
      break;
    case DOWN:
      check_collision(game_info, p, x, y + 1);
      if (p->isAlive == 1) {
        game_info->board[x][y + 1] = id;
        p->y += 1;
      }
      break;
    case LEFT:
      check_collision(game_info, p, x - 1, y);
      if (p->isAlive == 1) {
        game_info->board[x - 1][y] = id;
        p->x -= 1;
      }
      break;
    case RIGHT:
      check_collision(game_info, p, x + 1, y);
      if (p->isAlive == 1) {
        game_info->board[x + 1][y] = id;
        p->x += 1;
      }
      break;
  }

  if (p->trail_up == 1 && p->isAlive) {
    game_info->board[x][y] = 50 + id;
  } else if (p->isAlive) {
    game_info->board[x][y] = EMPTY;
  }
}

void update_game(display_info *game_info, player *joueurs, int nbr_players) {
  int i;
  game_info->winner = check_winner(game_info, joueurs, nbr_players);

  if (game_info->winner >= 0) {
    printf("🏆 player %d won\n", game_info->winner);
    game_running = 0;
  } else if (game_info->winner == TIE) {
    printf("🪢 Tie\n");
    game_running = 0;
  } else {
    for (i = 0; i < nbr_players; i++) {
      if (joueurs[i].isAlive == 1) {
        printf("Updating pos for player %d\n", joueurs[i].id);
        update_player_position(game_info, &joueurs[i]);
      }
    }
  }
}

void send_board_to_all_clients(display_info *game_info, int sockets[],
                               int nbr_sockets) {
  display_info board_copy;
  memcpy(&board_copy, game_info, sizeof(display_info));
  board_copy.winner = htonl(board_copy.winner);

  for (int i = 0; i < nbr_sockets; i++) {
    if (sockets[i] != 0) {
      CHECK(send(sockets[i], &board_copy, sizeof(display_info), 0));
    }
  }
}

void *fonction_thread_jeu(void *arg) {
  thread_arg *t_arg = (thread_arg *)arg;
  printf("thread jeu\n");

  while (1) {
    // update game
    if (*t_arg->game_running == 1) {
      update_game(t_arg->game_info, t_arg->joueurs, *t_arg->nbr_players);
      send_board_to_all_clients(t_arg->game_info, t_arg->sockets,
                                *t_arg->nbr_sockets);
      usleep(REFRESH_RATE * 1000);
    } else {
      pthread_exit(0);
    }
  }
}