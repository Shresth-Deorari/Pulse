// src/ui.c
#include "../include/ui.h"
#include <math.h>
#include <ncurses.h>
#include <string.h>

// --- Window pointers and UI state ---
static WINDOW *header_win, *cpu_win, *mem_win, *proc_win;
static int scroll_offset = 0;

// --- UI Constants & Color Definitions ---
#define HEADER_HEIGHT 1
#define MEM_PANEL_HEIGHT 3
#define CPU_ITEM_FIXED_WIDTH 18
#define HEADER_PAIR 1
#define PANEL_BORDER_PAIR 2
#define PROC_HEADER_PAIR 3
#define SCROLL_THUMB_PAIR 4

// --- Function Prototypes ---
void draw_header(void);
void draw_panel_border(WINDOW *win, const char *title);
void draw_cpu_panel(const double *cpu_usage, int num_total_cpu_entries);
void draw_mem_panel(const memStats *mem_info);
void draw_process_panel(const ProcessInfo *processes, int num_processes);

void ui_init(void) {
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  printf("\033[?1003h\n");

  if (has_colors()) {
    start_color();
    use_default_colors();
    init_pair(HEADER_PAIR, COLOR_BLACK, COLOR_WHITE);
    init_pair(PANEL_BORDER_PAIR, COLOR_WHITE, -1);
    init_pair(PROC_HEADER_PAIR, COLOR_BLACK, COLOR_CYAN);
    init_pair(SCROLL_THUMB_PAIR, COLOR_CYAN,
              COLOR_CYAN); // For the scrollbar thumb
  }
}

void ui_cleanup(void) {
  printf("\033[?1003l\n");
  mousemask(0, NULL);
  if (header_win)
    delwin(header_win);
  if (cpu_win)
    delwin(cpu_win);
  if (mem_win)
    delwin(mem_win);
  if (proc_win)
    delwin(proc_win);
  endwin();
}

void ui_handle_input(int ch, int num_processes) {
  if (!proc_win)
    return;
  int proc_win_height = getmaxy(proc_win);
  int drawable_height = proc_win_height - 3; // Space for processes
  if (drawable_height < 1)
    drawable_height = 1;

  int max_scroll = num_processes - drawable_height;
  if (max_scroll < 0)
    max_scroll = 0;

  if (ch == KEY_MOUSE) {
    MEVENT event;
    if (getmouse(&event) == OK) {
      if (event.bstate & BUTTON4_PRESSED) {
        if (scroll_offset > 0)
          scroll_offset--;
      } else if (event.bstate & BUTTON5_PRESSED) {
        if (scroll_offset < max_scroll)
          scroll_offset++;
      }
    }
  }

  switch (ch) {
  case KEY_UP:
    if (scroll_offset > 0)
      scroll_offset--;
    break;
  case KEY_DOWN:
    if (scroll_offset < max_scroll)
      scroll_offset++;
    break;
  }
}

void ui_draw(const double *cpu_usage, const memStats *mem_info,
             int num_total_cpu_entries, const ProcessInfo *processes,
             int num_processes) {
  int screen_width, screen_height;
  getmaxyx(stdscr, screen_height, screen_width);

  // Calculation and Window management is the same...
  int num_cpu_cols =
      (screen_width > 2) ? (screen_width - 2) / CPU_ITEM_FIXED_WIDTH : 1;
  if (num_cpu_cols == 0)
    num_cpu_cols = 1;
  int num_cpu_rows = (int)ceil((double)num_total_cpu_entries / num_cpu_cols);
  int cpu_win_height = num_cpu_rows + 2;
  int proc_win_height =
      screen_height - HEADER_HEIGHT - cpu_win_height - MEM_PANEL_HEIGHT;

  if (header_win)
    delwin(header_win);
  if (cpu_win)
    delwin(cpu_win);
  if (mem_win)
    delwin(mem_win);
  if (proc_win)
    delwin(proc_win);
  header_win = newwin(HEADER_HEIGHT, screen_width, 0, 0);
  cpu_win = newwin(cpu_win_height, screen_width, HEADER_HEIGHT, 0);
  mem_win =
      newwin(MEM_PANEL_HEIGHT, screen_width, HEADER_HEIGHT + cpu_win_height, 0);
  proc_win = newwin(proc_win_height, screen_width,
                    HEADER_HEIGHT + cpu_win_height + MEM_PANEL_HEIGHT, 0);

  // No need to werase(stdscr), as we are redrawing all sub-windows completely
  draw_header();
  draw_cpu_panel(cpu_usage, num_total_cpu_entries);
  draw_mem_panel(mem_info);
  draw_process_panel(processes, num_processes);

  wnoutrefresh(stdscr);
  wnoutrefresh(header_win);
  wnoutrefresh(cpu_win);
  wnoutrefresh(mem_win);
  wnoutrefresh(proc_win);
  doupdate();
}

void draw_header(void) {
  werase(header_win);
  int width = getmaxx(header_win);
  wbkgd(header_win, COLOR_PAIR(HEADER_PAIR));
  mvwprintw(header_win, 0, 1,
            "Pulse - Resource Monitor (Scroll with Arrow Keys/Mouse Wheel)");
  mvwprintw(header_win, 0, width - 12, "[q to quit]");
}

void draw_panel_border(WINDOW *win, const char *title) {
  wattron(win, COLOR_PAIR(PANEL_BORDER_PAIR));
  box(win, 0, 0);
  mvwprintw(win, 0, 2, " %s ", title);
  wattroff(win, COLOR_PAIR(PANEL_BORDER_PAIR));
}

void draw_cpu_panel(const double *cpu_usage, int num_total_cpu_entries) {
  werase(cpu_win); // FIX: Clear the window before drawing
  draw_panel_border(cpu_win, "CPU");
  // ... rest of function is unchanged ...
  int panel_width = getmaxx(cpu_win);
  int num_cols =
      (panel_width > 2) ? (panel_width - 2) / CPU_ITEM_FIXED_WIDTH : 1;
  if (num_cols == 0)
    num_cols = 1;
  int col_width = (panel_width - 2) / num_cols;
  for (int i = 0; i < num_total_cpu_entries; ++i) {
    int row = i / num_cols;
    int col = i % num_cols;
    char buffer[32];
    (i == 0) ? snprintf(buffer, sizeof(buffer), "Aggr: %.1f%%", cpu_usage[i])
             : snprintf(buffer, sizeof(buffer), "CPU%d: %.1f%%", i - 1,
                        cpu_usage[i]);
    mvwprintw(cpu_win, row + 1, col * col_width + 2, "%-*s", col_width, buffer);
  }
}

void draw_mem_panel(const memStats *mem_info) {
  werase(mem_win); // FIX: Clear the window before drawing
  draw_panel_border(mem_win, "Memory");
  // ... rest of function is unchanged ...
  unsigned long mem_used =
      mem_info->memTotal > 0 ? mem_info->memTotal - mem_info->memAvailable : 0;
  double mem_percent =
      mem_info->memTotal > 0 ? 100.0 * mem_used / mem_info->memTotal : 0.0;
  unsigned long swap_used =
      mem_info->swapTotal > 0 ? mem_info->swapTotal - mem_info->swapFree : 0;
  double swap_percent =
      mem_info->swapTotal > 0 ? 100.0 * swap_used / mem_info->swapTotal : 0.0;
  char mem_str[64], swap_str[64];
  snprintf(mem_str, sizeof(mem_str), "Mem: %lu/%lu KB (%.1f%%)", mem_used,
           mem_info->memTotal, mem_percent);
  snprintf(swap_str, sizeof(swap_str), "Swap: %lu/%lu KB (%.1f%%)", swap_used,
           mem_info->swapTotal, swap_percent);
  mvwprintw(mem_win, 1, 2, mem_str);
  mvwprintw(mem_win, 1, 4 + strlen(mem_str), swap_str);
}

void draw_process_panel(const ProcessInfo *processes, int num_processes) {
  werase(proc_win); // FIX: Clear the window before drawing
  draw_panel_border(proc_win, "Processes");
  int width = getmaxx(proc_win);
  int height = getmaxy(proc_win);
  int drawable_height = height - 3;
  if (drawable_height < 1)
    return;

  // --- Draw Header ---
  wattron(proc_win, COLOR_PAIR(PROC_HEADER_PAIR));
  mvwprintw(proc_win, 1, 1, "%-6s %-20s %-5s %-6s %-6s %-10s %-10s", "PID",
            "COMMAND", "S", "CPU%", "MEM%", "VIRT", "RES");
  for (int x = 80; x < width - 1; ++x)
    mvwaddch(proc_win, 1, x, ' ');
  wattroff(proc_win, COLOR_PAIR(PROC_HEADER_PAIR));

  // --- Draw Processes ---
  for (int i = 0; i < drawable_height; ++i) {
    int proc_index = scroll_offset + i;
    if (proc_index >= num_processes)
      break;
    const ProcessInfo *p = &processes[proc_index];
    char cmd[21];
    snprintf(cmd, sizeof(cmd), "%.20s", p->stats.comm);
    mvwprintw(proc_win, i + 2, 1, "%-6d %-20s %-5c %-6.1f %-6.1f %-10ld %-10ld",
              p->stats.pid, cmd, p->stats.state, p->cpu_percent, p->mem_percent,
              p->stats.vsize / 1024, p->stats.rss * 4);
  }

  // --- FIX: Draw the Scrollbar ---
  int max_scroll = num_processes - drawable_height;
  if (max_scroll > 0) {
    // Calculate thumb position
    double scroll_percent = (double)scroll_offset / max_scroll;
    int thumb_y = 2 + (int)(scroll_percent * (drawable_height - 1));

    // Draw the scrollbar track
    wattron(proc_win, A_DIM);
    mvwvline(proc_win, 2, width - 2, ACS_VLINE, drawable_height);
    wattroff(proc_win, A_DIM);

    // Draw the thumb
    wattron(proc_win, COLOR_PAIR(SCROLL_THUMB_PAIR));
    mvwaddch(proc_win, thumb_y, width - 2, ACS_BLOCK);
    wattroff(proc_win, COLOR_PAIR(SCROLL_THUMB_PAIR));
  }
}
