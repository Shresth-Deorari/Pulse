#include <ncurses.h>

int main(){

  initscr();
  noecho();
  cbreak();

  int x, y, begX, begY, maxX, maxY;

  getyx(stdscr, y, x);
  getbegyx(stdscr, begY, begX);
  getmaxyx(stdscr, maxY, maxX);
  printw( "X: %d" 
                "Y: %d", x, y);
  move(20, 20);
  printw( "Beg X: %d" 
                "Beg Y: %d", begX, begY);
getch();
  endwin();
}
