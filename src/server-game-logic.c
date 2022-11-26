#include <pthread.h>
#include <unistd.h>

#include "common.h"

#define BUF_SIZE 1024
#define MAX_JOUEURS 2

/// @brief Structure de données pour stocker les informations d'un joueur dans
/// une partie
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

void init_board(display_info *game_info, int maxX, int maxY);

player add_player(display_info *game_info, int id_player, int new_socket,
                  int id_sur_socket);

// supprime un joueur
void remove_player(display_info *game_info, player *list_joueurs,
                   int *nb_joueurs, int id_player);

void kill_player(player *player);

void reset_players(display_info *game_info, player *lst_joueurs, int maxX,
                   int maxY);

void restart(display_info *game_info, int maxX, int maxY, player *lst_joueurs,
             int *game_running);

int check_winner(display_info *game_info, player *joueurs, int nbr_players);

void update_player_direction(display_info *game_info, player *p, int direction);

void check_collision(display_info *game_info, player *p, int x, int y);

void update_player_position(display_info *game_info, player *p);

void update_game(display_info *game_info, player *joueurs, int nbr_players,
                 int *game_running);

void send_board_to_all_clients(display_info *game_info, int sockets[],
                               int nbr_sockets);

/// @brief Initialise le plateau de jeu avec des cases vides et des murs
/// @param game_info Pointeur vers la structure de données du jeu
/// @param maxX Nombre de colonnes du plateau
/// @param maxY Nombre de lignes du plateau
void init_board(display_info *game_info, int maxX, int maxY) {
  debug("init board\n");
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
  debug("add_player %d\n", id_player);
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
void remove_player(display_info *game_info, player *list_joueurs,
                   int *nb_joueurs, int id_player) {
  debug("remove_player %d\n", id_player);
}

void kill_player(player *player) { player->isAlive = 0; }

void reset_players(display_info *game_info, player *lst_joueurs, int maxX,
                   int maxY) {
  int i;
  for (i = 0; i < MAX_JOUEURS; i++) {
    lst_joueurs[i].isAlive = 1;
    switch (lst_joueurs[i].id) {
      case 0:
        game_info->board[1][1] = lst_joueurs[i].id;
        lst_joueurs[i].x = 1;
        lst_joueurs[i].y = 1;
        lst_joueurs[i].direction = DOWN;
        break;
      case 1:
        game_info->board[maxX - 2][maxY - 2] = lst_joueurs[i].id;
        lst_joueurs[i].x = maxX - 2;
        lst_joueurs[i].y = maxY - 2;
        lst_joueurs[i].direction = UP;
        break;
    }
  }
}

void restart(display_info *game_info, int maxX, int maxY, player *lst_joueurs,
             int *game_running) {
  *game_running = 0;
  init_board(game_info, maxX, maxY);
  reset_players(game_info, lst_joueurs, maxX, maxY);
  *game_running = 1;
}

int check_winner(display_info *game_info, player *joueurs, int nbr_players) {
  // si un joueur est mort, on verifie qu'un autre ne serait pas mort au
  // meme moment
  debug("CheckWinner\n");
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
  // gauche il peut pas directement aller a droite comme dans le snake
  // classique
  debug("update_player_direction\n");
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
  debug("check_collision\n");
  if (game_info->board[x][y] != EMPTY) {
    debug("🚨 player %d collided\n", p->id);
    kill_player(p);
  }
}

void update_player_position(display_info *game_info, player *p) {
  debug("Update Player\n");
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

void update_game(display_info *game_info, player *joueurs, int nbr_players,
                 int *game_running) {
  debug("update_game\n");
  int i;
  game_info->winner = check_winner(game_info, joueurs, nbr_players);

  if (game_info->winner >= 0) {
    debug("🏆 player %d won\n", game_info->winner);
    *game_running = 0;
  } else if (game_info->winner == TIE) {
    debug("🪢 Tie\n");
    *game_running = 0;
  } else {
    for (i = 0; i < nbr_players; i++) {
      if (joueurs[i].isAlive == 1) {
        debug("Updating pos for player %d\n", joueurs[i].id);
        update_player_position(game_info, &joueurs[i]);
      }
    }
  }
}

void send_board_to_all_clients(display_info *game_info, int sockets[],
                               int nbr_sockets) {
  debug("sendboardtoall\n");
  debug("%d\n", nbr_sockets);
  for (int i = 0; i < nbr_sockets; i++) {
    debug("check socket %d\n", i);
    if (sockets[i] != 0) {
      debug("sendign to %d\n", i);
      send(sockets[i], game_info, sizeof(display_info), 0);
    }
  }

  debug("t2\n");
}
