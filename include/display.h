#include <ncurses.h>
#include <termios.h>

#include "./common.h"

#define NB_COLORS 5
#define TRAIL_INDEX_SHIFT 50

#define BLUE_ON_BLACK 0
#define RED_ON_BLACK 2
#define YELLOW_ON_BLACK 1
#define MAGENTA_ON_BLACK 3
#define CYAN_ON_BLACK 4

#define BLUE_ON_BLUE 50
#define RED_ON_RED 52
#define YELLOW_ON_YELLOW 51
#define MAGENTA_ON_MAGENTA 53
#define CYAN_ON_CYAN 54

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

void display_character(int color, int y, int x, char character) {
  attron(COLOR_PAIR(color));
  mvaddch(y, x, character);
  attroff(COLOR_PAIR(color));
}

void update_display() {
  clear();
  refresh();
}