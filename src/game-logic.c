#include "../include/game-logic.h"

void init_board(display_info *game_info) {
  int x, y;
  for (x = 0; x < XMAX; x++) {
    for (y = 0; y < YMAX; y++) {
      if (x == 0 || x == XMAX - 1 || y == 0 || y == YMAX - 1) {
        game_info->board[x][y] = WALL;
      } else {
        game_info->board[x][y] = EMPTY;
      }
    }
  }
  game_info->winner = -1;
}

player add_player(display_info *game_info, initial_player_position *init,
                  int id_player, int new_socket, int id_on_socket) {
  player new_player;

  game_info->board[init[id_player].x][init[id_player].y] = id_player;
  new_player.direction = init[id_player].direction;
  new_player.x = init[id_player].x;
  new_player.y = init[id_player].y;

  new_player.id = id_player;
  new_player.trail_is_enabled = 1;
  new_player.is_alive = 1;
  new_player.socket_associated = new_socket;
  new_player.id_on_socket = id_on_socket;

  return new_player;
}

void kill_player(player *player) { player->is_alive = 0; }

void reset_players(display_info *game_info, initial_player_position *init,
                   player *players_list) {
  int i;
  for (i = 0; i < MAX_PLAYERS; i++) {
    game_info->board[init[i].x][init[i].y] = i;
    players_list[i].direction = init[i].direction;
    players_list[i].x = init[i].x;
    players_list[i].y = init[i].y;

    players_list[i].is_alive = 1;
    players_list[i].trail_is_enabled = 1;
  }
}

void restart(display_info *game_info, initial_player_position *init,
             player *players_list, int *game_running) {
  *game_running = 0;
  init_board(game_info);
  reset_players(game_info, init, players_list);
  *game_running = 1;
}

void check_collision(display_info *game_info, player *p, int x, int y) {
  // Si l'emplacement du joueur n'est pas vide, il doit mourir
  if (game_info->board[x][y] != EMPTY) {
    kill_player(p);
  }
}

void update_player_position(display_info *game_info, player *p) {
  int x = p->x;
  int y = p->y;
  int id = p->id;

  // Mettre a jour la position du joueur tout en verifiant les collisions
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

  // Laisser le mur de lumiere derriere le joueur s'il est actif
  if (p->trail_is_enabled) {
    game_info->board[x][y] = 50 + id;
  } else {
    game_info->board[x][y] = EMPTY;
  }
}

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
  } else if (players_list[0].is_alive == 0) {
    return players_list[1].id;
  } else if (players_list[1].is_alive == 0) {
    return players_list[0].id;
  }

  return -1;
}

void update_player_direction(player *players_list, int player_count,
                             int socket_associated, int id_on_socket,
                             int direction) {
  // Chercher le joueur qui correspond au couple socket/id_sur_socket
  player *p;
  int player_found = 0;
  int j;

  for (j = 0; j < player_count; j++) {
    if (players_list[j].socket_associated == socket_associated &&
        players_list[j].id_on_socket == id_on_socket) {
      p = &players_list[j];
      player_found = 1;
    }
  }

  // Si le joueur n'a pas ete trouve, on ignore. (par exemple input du deuxieme
  // joueur sur un socket qui n'en contient qu'un)
  if (player_found == 0) {
    return;
  }

  // Modifier la direction du joueur (on autorise pas de revenir en arriere,
  // c'est a dire si le joueur va a gauche il peut pas directement aller a
  // droite comme dans le snake classique)
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

void update_game(display_info *game_info, player *players_list,
                 int player_count, int *game_running) {
  int i;
  game_info->winner = check_winner(game_info, players_list, player_count);

  // S'il y a un gagnant ou une egalite, arreter le jeu
  if (game_info->winner >= 0) {
    *game_running = 0;
  } else if (game_info->winner == TIE) {
    *game_running = 0;
  } else {
    // Sinon, mettre a jour la position des joueurs
    for (i = 0; i < player_count; i++) {
      if (players_list[i].is_alive == 1) {
        update_player_position(game_info, &players_list[i]);
      }
    }
  }
}

void send_board_to_all_clients(display_info *game_info, int sockets_list[],
                               int socket_count) {
  for (int i = 0; i < socket_count; i++) {
    if (sockets_list[i] != 0) {
      send(sockets_list[i], game_info, sizeof(display_info), 0);
    }
  }
}