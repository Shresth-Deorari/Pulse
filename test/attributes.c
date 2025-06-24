#include<ncurses.h>

int main(){
  initscr();
  if(!has_colors()){
    printw("Terminal doesn't have colours");
    getch();
    return -1;
  }
  start_color();
  if(can_change_color()){
    printw("Can change color");
    init_color(COLOR_BLACK, 70, 94, 29);
    refresh();
  }  
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_BLUE, COLOR_CYAN);
  move(20, 20);
  attron(COLOR_PAIR(1));
  printw("This is new text");
  attron(COLOR_PAIR(2));
  printw("New text?");
  getch();
  endwin();
  return 0;
}
