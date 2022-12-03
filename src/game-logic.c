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

void add_player(player *players_list, display_info *game_info,
                initial_player_position *init, int id_player, int new_socket,
                int id_on_socket) {
  player new_player;

  new_player.direction = init[id_player].direction;
  new_player.x = init[id_player].x;
  new_player.y = init[id_player].y;
  new_player.id = id_player;
  new_player.trail_is_enabled = 1;
  new_player.is_alive = 1;
  new_player.socket_associated = new_socket;
  new_player.id_on_socket = id_on_socket;

  game_info->board[init[id_player].x][init[id_player].y] = id_player;
  players_list[id_player] = new_player;
}

void kill_player(player *player) { player->is_alive = 0; }

void reset_players(display_info *game_info, initial_player_position *init,
                   player *players_list, int max_players) {
  for (int i = 0; i < max_players; i++) {
    add_player(players_list, game_info, init, i,
               players_list[i].socket_associated, players_list[i].id_on_socket);
  }
}

void restart_game(display_info *game_info, initial_player_position *init,
                  player *players_list, int *game_running, int max_players) {
  init_board(game_info);
  reset_players(game_info, init, players_list, max_players);
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
      if (p->is_alive) {
        game_info->board[x][y - 1] = id;
        p->y--;
      }
      break;
    case DOWN:
      check_collision(game_info, p, x, y + 1);
      if (p->is_alive) {
        game_info->board[x][y + 1] = id;
        p->y += 1;
      }
      break;
    case LEFT:
      check_collision(game_info, p, x - 1, y);
      if (p->is_alive) {
        game_info->board[x - 1][y] = id;
        p->x -= 1;
      }
      break;
    case RIGHT:
      check_collision(game_info, p, x + 1, y);
      if (p->is_alive) {
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

int check_winner(player *players_list, int player_count) {
  // On verifie le nombre de joueurs encore en vie
  int alive_count = 0;
  int alive_id = -1;
  for (int i = 0; i < player_count; i++) {
    if (players_list[i].is_alive) {
      alive_count++;
      alive_id = i;
    }
  }

  if (alive_count == 1) {
    return alive_id;  // 1 joueur en vie = gagnant
  } else if (alive_count == 0) {
    return -2;  // 0 joueur en vie = egalite
  } else {
    return -1;  // Plusieurs joueurs en vie = pas de gagnant
  }
}

void update_player_direction(player *players_list, int player_count,
                             int socket_associated, int id_on_socket,
                             int direction) {
  // Chercher le joueur qui correspond au couple socket/id_sur_socket
  player *p;
  int player_found = 0;

  for (int i = 0; i < player_count; i++) {
    if (players_list[i].socket_associated == socket_associated &&
        players_list[i].id_on_socket == id_on_socket) {
      p = &players_list[i];
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
  game_info->winner = check_winner(players_list, player_count);

  // S'il y a un gagnant ou une egalite, arreter le jeu
  if (game_info->winner >= 0) {
    *game_running = 0;
  } else if (game_info->winner == TIE) {
    *game_running = 0;
  } else {
    // Sinon, mettre a jour la position des joueurs
    for (int i = 0; i < player_count; i++) {
      if (players_list[i].is_alive) {
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