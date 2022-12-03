#include <ncurses.h>
#include <termios.h>

#include "common.h"

#define NB_COLORS 5
#define TRAIL_INDEX_SHIFT 50

#define BLUE_ON_BLACK 0
#define YELLOW_ON_BLACK 1
#define RED_ON_BLACK 2
#define MAGENTA_ON_BLACK 3
#define CYAN_ON_BLACK 4

#define BLUE_ON_BLUE 50
#define YELLOW_ON_YELLOW 51
#define RED_ON_RED 52
#define MAGENTA_ON_MAGENTA 53
#define CYAN_ON_CYAN 54

#define KEY_UP_P1 'z'
#define KEY_DOWN_P1 's'
#define KEY_LEFT_P1 'q'
#define KEY_RIGHT_P1 'd'
#define KEY_TRAIL_P1 ' '
#define KEY_UP_P2 'i'
#define KEY_DOWN_P2 'k'
#define KEY_LEFT_P2 'j'
#define KEY_RIGHT_P2 'l'
#define KEY_TRAIL_P2 'm'
#define KEY_QUIT 'x'

/// @brief Met a jour l'affichage du jeu
/// @param game_info Informations du jeu
void update_display(display_info *game_info);

/// @brief Affiche un message au centre de l'ecran
/// @param message Message a afficher
void show_centered_message(char *message);

// Fonctions fournies par le professeur
void display_character(int color, int y, int x, char character);
void tune_terminal();
void init_graphics();