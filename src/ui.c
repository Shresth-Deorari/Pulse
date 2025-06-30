#include "../include/ui.h"
#include <math.h>
#include <ncurses.h>
#include <string.h>

static WINDOW *header_win, *cpu_win, *mem_win;

#define HEADER_HEIGHT 1
#define MEM_WIN_HEIGHT 4
#define MIN_WIDTH_PER_CPU_METER 22

#define HEADER_PAIR 1
#define METER_BAR_PAIR 2
#define METER_TEXT_PAIR 3
#define PANEL_BORDER_PAIR 4

void draw_header(void);
void draw_panel_border(WINDOW *win, const char *title);
void draw_meter(WINDOW *win, int y, int x, int width, double percent);
void draw_cpu_panel(const double *cpu_usage, int num_total_cpu_entries);
void draw_mem_panel(const memStats *mem_info);

void ui_init(void) {
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);

  if (has_colors()) {
    start_color();
    use_default_colors();
    init_pair(HEADER_PAIR, COLOR_BLACK, COLOR_WHITE);
    init_pair(METER_BAR_PAIR, COLOR_CYAN, COLOR_CYAN);
    init_pair(METER_TEXT_PAIR, COLOR_WHITE, -1);
    init_pair(PANEL_BORDER_PAIR, COLOR_WHITE, -1);
  }
}

void ui_cleanup(void) {
  if (header_win)
    delwin(header_win);
  if (cpu_win)
    delwin(cpu_win);
  if (mem_win)
    delwin(mem_win);
  endwin();
}

void ui_handle_input(int ch) { (void)ch; }

void ui_draw(const double *cpu_usage, const memStats *mem_info,
             int num_total_cpu_entries) {
  int screen_width, screen_height;
  getmaxyx(stdscr, screen_height, screen_width);

  int num_cpu_cols = (screen_width - 2) / MIN_WIDTH_PER_CPU_METER;
  if (num_cpu_cols == 0)
    num_cpu_cols = 1;

  int num_cpu_rows = (int)ceil(((double)num_total_cpu_entries / num_cpu_cols));
  int cpu_win_height = 2 + num_cpu_rows; // 2 for border, 1 row per.. well, row.

  if (header_win)
    delwin(header_win);
  if (cpu_win)
    delwin(cpu_win);
  if (mem_win)
    delwin(mem_win);

  header_win = newwin(HEADER_HEIGHT, screen_width, 0, 0);
  cpu_win = newwin(cpu_win_height, screen_width, HEADER_HEIGHT, 0);
  mem_win =
      newwin(MEM_WIN_HEIGHT, screen_width, HEADER_HEIGHT + cpu_win_height, 0);

  werase(stdscr);
  draw_header();
  draw_cpu_panel(cpu_usage, num_total_cpu_entries);
  draw_mem_panel(mem_info);

  wnoutrefresh(stdscr);
  wnoutrefresh(header_win);
  wnoutrefresh(cpu_win);
  wnoutrefresh(mem_win);
  doupdate();
}

void draw_header(void) {
  int width = getmaxx(header_win);
  wbkgd(header_win, COLOR_PAIR(HEADER_PAIR));
  mvwprintw(header_win, 0, 1, "Pulse - Resource Monitor");
  wattron(header_win, A_BOLD);
  mvwprintw(header_win, 0, width - 12, "[q to quit]");
  wattroff(header_win, A_BOLD);
}

void draw_panel_border(WINDOW *win, const char *title) {
  wattron(win, COLOR_PAIR(PANEL_BORDER_PAIR));
  box(win, 0, 0);
  mvwprintw(win, 0, 2, " %s ", title);
  wattroff(win, COLOR_PAIR(PANEL_BORDER_PAIR));
}

void draw_meter(WINDOW *win, int y, int x, int width, double percent) {
  if (width <= 2)
    return;

  int bar_content_width = width - 2;

  char text[16];
  snprintf(text, sizeof(text), "%.1f%%", percent);
  int text_len = strlen(text);

  mvwaddch(win, y, x, '[');
  mvwaddch(win, y, x + width - 1, ']');

  int text_pos = x + 1 + (bar_content_width - text_len) / 2;
  if (text_pos + text_len >= x + width) {
    text_pos = x + width - 1 - text_len;
  }
  if (text_pos < x + 1) {
    text_pos = x + 1;
  }

  wattron(win, COLOR_PAIR(METER_TEXT_PAIR) | A_BOLD);
  mvwprintw(win, y, text_pos, "%s", text);
  wattroff(win, COLOR_PAIR(METER_TEXT_PAIR) | A_BOLD);
}

void draw_cpu_panel(const double *cpu_usage, int num_total_cpu_entries) {
  draw_panel_border(cpu_win, "CPU");
  int panel_width = getmaxx(cpu_win);

  int num_cols = (panel_width - 2) / MIN_WIDTH_PER_CPU_METER;
  if (num_cols == 0)
    num_cols = 1;

  int col_content_width = (panel_width - 2) / num_cols;
  int meter_width = col_content_width - 7; // 7 chars for "CPUxx: "

  if (meter_width <= 2)
    return; // Not enough space

  for (int i = 0; i < num_total_cpu_entries; ++i) {
    int row = i / num_cols;
    int col = i % num_cols;

    int draw_y = 1 + row;
    int draw_x = 2 + col * col_content_width;

    char core_label[8];
    if (i == 0) {
      snprintf(core_label, sizeof(core_label), "Aggr  : ");
    } else {
      snprintf(core_label, sizeof(core_label), "CPU%-3d:", i - 1);
    }
    mvwprintw(cpu_win, draw_y, draw_x, "%s", core_label);

    draw_meter(cpu_win, draw_y, draw_x + 7, meter_width, cpu_usage[i]);
  }
}

void draw_mem_panel(const memStats *mem_info) {
  draw_panel_border(mem_win, "Memory & Swap");
  int panel_width = getmaxx(mem_win)/2;
  int meter_width = panel_width / 2 - 10;

  double mem_percent = 0.0;
  unsigned long mem_used = 0;
  if (mem_info->memTotal > 0) {
    mem_used = mem_info->memTotal - mem_info->memAvailable;
    mem_percent = 100.0 * mem_used / mem_info->memTotal;
  }
  mvwprintw(mem_win, 1, 2, "Mem[");
  mvwprintw(mem_win, 1, panel_width - 1, "]");
  mvwprintw(mem_win, 1, 7, "%9lu/%-9luKB", mem_used, mem_info->memTotal);
  draw_meter(mem_win, 1, panel_width / 2, meter_width, mem_percent);

  double swap_percent = 0.0;
  unsigned long swap_used = 0;
  if (mem_info->swapTotal > 0) {
    swap_used = mem_info->swapTotal - mem_info->swapFree;
    swap_percent = 100.0 * swap_used / mem_info->swapTotal;
  }
  mvwprintw(mem_win, 2, 2, "Swp [");
  mvwprintw(mem_win, 2, panel_width - 2, "]");
  mvwprintw(mem_win, 2, 7, "%9lu/%-9luKB", swap_used, mem_info->swapTotal);
  draw_meter(mem_win, 2, panel_width / 2, meter_width, swap_percent);
}
