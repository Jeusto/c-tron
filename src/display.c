#include "../include/display.h"

void update_display(display_info *game_info) {
  clear();

  // Afficher le plateau
  for (int x = 0; x < XMAX; x++) {
    for (int y = 0; y < YMAX; y++) {
      char color = game_info->board[x][y];

      if (color >= 0 && color < NB_COLORS) {
        display_character(color, y, x, '0' + color + 1);
      } else if (color == EMPTY) {
        display_character(color, y, x, ' ');
      } else {
        display_character(color, y, x, color);
      }
    }
  }

  // Afficher le resultat si la partie est finie
  if (game_info->winner != NO_WINNER) {
    clear();

    if (game_info->winner == TIE) {
      mvprintw(YMAX / 2, XMAX / 8, "Egalite!");
    } else {
      attron(COLOR_PAIR(game_info->winner));
      mvprintw(YMAX / 2, XMAX / 8, "Le joueur %d a gagne!",
               game_info->winner + 1);
      attroff(COLOR_PAIR(game_info->winner));
    }

    mvprintw(YMAX / 2 + 2, XMAX / 8,
             "Veuillez attendre que le serveur redemarre une partie ou appuyez "
             "sur 'x' pour quitter.");
  }

  refresh();
}

void show_centered_message(char *message) {
  clear();
  mvprintw(YMAX / 2, XMAX / 8, "%s", message);
  refresh();
}

// Fonctions fournies par le professeur
void display_character(int color, int y, int x, char character) {
  attron(COLOR_PAIR(color));
  mvaddch(y, x, character);
  attroff(COLOR_PAIR(color));
}
void tune_terminal() {
  struct termios term;
  tcgetattr(0, &term);
  term.c_iflag &= ~ICANON;
  term.c_lflag &= ~ICANON;
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &term);
}
void init_graphics() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(100);
  start_color();
  init_pair(BLUE_ON_BLACK, COLOR_BLUE, COLOR_BLACK);
  init_pair(RED_ON_BLACK, COLOR_RED, COLOR_BLACK);
  init_pair(YELLOW_ON_BLACK, COLOR_YELLOW, COLOR_BLACK);
  init_pair(MAGENTA_ON_BLACK, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(CYAN_ON_BLACK, COLOR_CYAN, COLOR_BLACK);

  init_pair(BLUE_ON_BLUE, COLOR_BLUE, COLOR_BLUE);
  init_pair(RED_ON_RED, COLOR_RED, COLOR_RED);
  init_pair(YELLOW_ON_YELLOW, COLOR_YELLOW, COLOR_YELLOW);
  init_pair(MAGENTA_ON_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA);
  init_pair(CYAN_ON_CYAN, COLOR_CYAN, COLOR_CYAN);

  init_pair(WALL, COLOR_WHITE, COLOR_WHITE);
}