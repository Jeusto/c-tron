#include "common.h"

/// @brief Structure de données pour stocker les infos d'un joueur dans le jeu
typedef struct player {
  int id;
  int x;
  int y;
  int trail_is_enabled;
  int direction;
  int is_alive;
  int socket_associated;
  int id_on_socket;
} player;

/// @brief Initialise le plateau de jeu avec des cases vides et des murs
/// @param game_info Informations du jeu
/// @param maxX Nombre de colonnes du plateau
/// @param maxY Nombre de lignes du plateau
void init_board(display_info *game_info, int maxX, int maxY) {
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

/// @brief Ajoute un joueur au jeu
/// @param game_info Informations du jeu
/// @param id_player Id du joueur a ajouter
/// @param new_socket Socket associe au joueur
/// @param id_on_socket Id du joueur sur le socket a laquelle il est associe
/// @return Le joueur ajouté
player add_player(display_info *game_info, int id_player, int new_socket,
                  int id_on_socket) {
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
  new_player.trail_is_enabled = 1;
  new_player.is_alive = 1;
  new_player.socket_associated = new_socket;
  new_player.id_on_socket = id_on_socket;

  return new_player;
}

/// @brief Supprime un joueur du jeu
/// @param game_info Informations du jeu
/// @param players_list Liste des joueurs
/// @param player_count Nombre de joueurs
/// @param id_player Id du joueur à supprimer
void remove_player(display_info *game_info, player *players_list,
                   int *player_count, int id_player) {}

/// @brief Tue un joueur
/// @param player Le joueur à tuer
void kill_player(player *player) { player->is_alive = 0; }

/// @brief Réinitialise les joueurs
/// @param game_info Informations du jeu
/// @param players_list Liste des joueurs
/// @param maxX Nombre de colonnes du plateau
/// @param maxY Nombre de lignes du plateau
void reset_players(display_info *game_info, player *players_list, int maxX,
                   int maxY) {
  int i;
  for (i = 0; i < MAX_PLAYERS; i++) {
    players_list[i].is_alive = 1;
    switch (players_list[i].id) {
      case 0:
        game_info->board[1][1] = players_list[i].id;
        players_list[i].x = 1;
        players_list[i].y = 1;
        players_list[i].direction = DOWN;
        break;
      case 1:
        game_info->board[maxX - 2][maxY - 2] = players_list[i].id;
        players_list[i].x = maxX - 2;
        players_list[i].y = maxY - 2;
        players_list[i].direction = UP;
        break;
    }
  }
}

/// @brief Redémarre le jeu
/// @param game_info Informations du jeu
/// @param maxX Nombre de colonnes du plateau
/// @param maxY Nombre de lignes du plateau
/// @param players_list Liste des joueurs
/// @param game_running Ptr vers la variable indiquant si le jeu est en cours
void restart(display_info *game_info, int maxX, int maxY, player *players_list,
             int *game_running) {
  *game_running = 0;
  init_board(game_info, maxX, maxY);
  reset_players(game_info, players_list, maxX, maxY);
  *game_running = 1;
}

/// @brief Verifie si un joueur est en collision avec un autre joueur ou les
/// murs et le tue si c'est le cas
/// @param game_info Informations du jeu
/// @param p Joueur
/// @param x Position x
/// @param y Position y
void check_collision(display_info *game_info, player *p, int x, int y) {
  if (game_info->board[x][y] != EMPTY) {
    kill_player(p);
  }
}

/// @brief Met a jour la position du joueur
/// @param game_info Informations du jeu
/// @param p Joueur dont la position doit etre mise a jour
void update_player_position(display_info *game_info, player *p) {
  int x = p->x;
  int y = p->y;
  int id = p->id;

  // remove all trail if trail_is_enabled == 0
  if (p->trail_is_enabled == 0) {
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
      if (p->is_alive == 1) {
        game_info->board[x][y - 1] = id;
        p->y--;
      }
      break;
    case DOWN:
      check_collision(game_info, p, x, y + 1);
      if (p->is_alive == 1) {
        game_info->board[x][y + 1] = id;
        p->y += 1;
      }
      break;
    case LEFT:
      check_collision(game_info, p, x - 1, y);
      if (p->is_alive == 1) {
        game_info->board[x - 1][y] = id;
        p->x -= 1;
      }
      break;
    case RIGHT:
      check_collision(game_info, p, x + 1, y);
      if (p->is_alive == 1) {
        game_info->board[x + 1][y] = id;
        p->x += 1;
      }
      break;
  }

  if (p->trail_is_enabled == 1 && p->is_alive) {
    game_info->board[x][y] = 50 + id;
  } else if (p->is_alive) {
    game_info->board[x][y] = EMPTY;
  }
}

/// @brief Vérifie si un joueur a gagné
/// @param game_info Informations du jeu
/// @param players_list Liste des joueurs
/// @param player_count Nombre de joueurs
/// @return 1 si un joueur a gagné, -2 si il y a égalité, -1 sinon
int check_winner(display_info *game_info, player *players_list,
                 int player_count) {
  // Si un joueur est mort, on verifie qu'un autre ne serait pas mort au meme
  // moment (et on aurait donc une egalite)
  if (!players_list[0].is_alive || !players_list[1].is_alive) {
    int i;
    for (i = 0; i < player_count; i++) {
      if (players_list[i].is_alive) {
        update_player_position(game_info, &players_list[i]);
      }
    }
  }

  if (players_list[0].is_alive == 0 && players_list[1].is_alive == 0) {
    return TIE;
  }
  if (players_list[0].is_alive == 0) {
    return players_list[1].id;
  }
  if (players_list[1].is_alive == 0) {
    return players_list[0].id;
  }

  return -1;
}

/// @brief Met à jour la position d'un joueur
/// @param game_info Informations du jeu
/// @param p Joueur dont la direction doit être mise à jour
/// @param direction La nouvelle direction du joueur
void update_player_direction(display_info *game_info, player *p,
                             int direction) {
  // on autorise pas de revenir en arriere, c'est a dire si le joueur va a
  // gauche il peut pas directement aller a droite comme dans le snake
  // classique
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
      p->trail_is_enabled = !p->trail_is_enabled;
      break;
  }
}

/// @brief Met a jour le jeu en deplacant les joueurs et en verifiant s'il y a
/// un gagnant
/// @param game_info Informations du jeu
/// @param players_list Liste des joueurs
/// @param player_count Nombre de joueurs
/// @param game_running Variable qui indique si le jeu est en cours
void update_game(display_info *game_info, player *players_list,
                 int player_count, int *game_running) {
  int i;
  game_info->winner = check_winner(game_info, players_list, player_count);

  if (game_info->winner >= 0) {
    *game_running = 0;
  } else if (game_info->winner == TIE) {
    *game_running = 0;
  } else {
    for (i = 0; i < player_count; i++) {
      if (players_list[i].is_alive == 1) {
        update_player_position(game_info, &players_list[i]);
      }
    }
  }
}

/// @brief Envoie les informations du jeu a tout les clients connectes
/// @param game_info Informations du jeu Informations du jeu
/// @param sockets_list Sockets des clients connectes
/// @param socket_count Nombre de sockets
void send_board_to_all_clients(display_info *game_info, int sockets_list[],
                               int socket_count) {
  for (int i = 0; i < socket_count; i++) {
    if (sockets_list[i] != 0) {
      send(sockets_list[i], game_info, sizeof(display_info), 0);
    }
  }
}
