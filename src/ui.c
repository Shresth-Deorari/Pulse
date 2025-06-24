#include "../include/ui.h"
#include <ncurses.h>

void ui_init(void) {
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, true);
  curs_set(0);
}

void ui_cleanup(void) { endwin(); }

void ui_draw(void) {
  int maxX, maxY;
  getmaxyx(stdscr, maxY, maxX);

  box(stdscr, 0, 0);

  mvprintw(1, (maxX -13)/2, "Pulse v2.0");

  mvprintw(maxY/2, (maxX - 28)/2, "Press q to quit the app.");

  refresh();
}
