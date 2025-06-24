#include <ncurses.h>

int main() {
  initscr();
  cbreak();
  noecho();
  int maxX, maxY;
  getmaxyx(stdscr, maxY, maxX);
  WINDOW *menu = newwin(5, maxX - 10, 20, 5);

  keypad(menu, TRUE);

  refresh();

  box(menu, 0, 0);
  int choice;
  wrefresh(menu);

  char *list[3] = {"Shresth", "Jiya", "Aadi"};
  int highlighted = 0;
  while (1) {
    for (int i = 0; i < 3; i++) {
      if (i == highlighted)
        wattron(menu, A_REVERSE);
      mvwprintw(menu, i + 1, 1, "%s", list[i]);
      wattroff(menu, A_REVERSE);
      wrefresh(menu);
    }
    choice = wgetch(menu);
    switch (choice) {
    case KEY_UP:
      highlighted--;
      if (highlighted < 0)
        highlighted = 0;
      break;
    case KEY_DOWN:
      highlighted++;
      if (highlighted > 2)
        highlighted = 2;
      break;
    default:
      break;
    }
    if (choice == 10)
      break;
  }

  getch();
  endwin();
}
