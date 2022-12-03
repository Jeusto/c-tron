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

/// @brief Structure de données pour simplifier l'initialisation des joueurs
typedef struct initial_player_position {
  int x;
  int y;
  int direction;
} initial_player_position;

/// @brief Initialise le plateau de jeu avec des cases vides et des murs
/// @param game_info Informations du jeu
void init_board(display_info *game_info);

/// @brief Ajoute un joueur au jeu
/// @param players_list Liste des joueurs
/// @param game_info Informations du jeu
/// @param init Positions initiales des joueurs
/// @param id_player Id du joueur a ajouter
/// @param new_socket Socket associe au joueur
/// @param id_on_socket Id du joueur sur le socket a laquelle il est associe
/// @return Le nouveau joueur cree
void add_player(player *players_list, display_info *game_info,
                initial_player_position *init, int id_player, int new_socket,
                int id_on_socket);

/// @brief Tue un joueur
/// @param player Le joueur à tuer
void kill_player(player *player);

/// @brief Réinitialise les joueurs et leur position dans le plateau
/// @param game_info Informations du jeu
/// @param init Positions initiales des joueurs
/// @param players_list Liste des joueurs
void reset_players(display_info *game_info, initial_player_position *init,
                   player *players_list, int max_players);

/// @brief Redémarre le jeu
/// @param game_info Informations du jeu
/// @param init Positions initiales des joueurs
/// @param players_list Liste des joueurs
/// @param game_running Ptr vers la variable indiquant si le jeu est en cours
void restart_game(display_info *game_info, initial_player_position *init,
                  player *players_list, int *game_running, int max_players);

/// @brief Verifie si un joueur est en collision avec un autre joueur ou les
/// murs et le tue si c'est le cas
/// @param game_info Informations du jeu
/// @param p Joueur
/// @param x Position x
/// @param y Position y
void check_collision(display_info *game_info, player *p, int x, int y);

/// @brief Met a jour la position du joueur
/// @param game_info Informations du jeu
/// @param p Joueur dont la position doit etre mise a jour
void update_player_position(display_info *game_info, player *p);

/// @brief Vérifie si un joueur a gagné
/// @param players_list Liste des joueurs
/// @param player_count Nombre de joueurs
/// @return 1 si un joueur a gagné, -2 si il y a égalité, -1 sinon
int check_winner(player *players_list, int player_count);

/// @brief Met à jour la position d'un joueur
/// @param p Joueur dont la direction doit être mise à jour
/// @param direction La nouvelle direction du joueur
void update_player_direction(player *players_list, int player_count,
                             int socket_associated, int id_on_socket,
                             int direction);

/// @brief Met a jour le jeu en deplacant les joueurs et en verifiant s'il y a
/// un gagnant
/// @param game_info Informations du jeu
/// @param players_list Liste des joueurs
/// @param player_count Nombre de joueurs
/// @param game_running Variable qui indique si le jeu est en cours
void update_game(display_info *game_info, player *players_list,
                 int player_count, int *game_running);

/// @brief Envoie les informations du jeu a tout les clients connectes
/// @param game_info Informations du jeu Informations du jeu
/// @param sockets_list Sockets des clients connectes
/// @param socket_count Nombre de sockets
void send_board_to_all_clients(display_info *game_info, int sockets_list[],
                               int socket_count);