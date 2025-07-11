#include "../include/ui.h"
#include <ncurses.h>
#include <string.h>

static WINDOW *header_win, *cpu_win, *mem_win, *proc_win;
static int scroll_offset = 0;

#define HEADER_HEIGHT 1
#define MEM_PANEL_HEIGHT 3
#define CPU_ITEM_FIXED_WIDTH 18
#define HEADER_PAIR 1
#define PANEL_BORDER_PAIR 2
#define PROC_HEADER_PAIR 3
#define SCROLL_THUMB_PAIR 4

void draw_header(void);
void draw_panel_border(WINDOW *win, const char *title);
void draw_cpu_panel(const double *cpu_usage, int num_total_cpu_entries);
void draw_mem_panel(const memStats *mem_info);
void draw_process_panel(const ProcessInfo *processes, int num_processes);
static void format_memory_unit(char *buf, size_t buf_size, long kb);

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
    init_pair(SCROLL_THUMB_PAIR, COLOR_CYAN, COLOR_CYAN);
  }
  ui_resize();
}

void ui_resize(void) {
  endwin();
  refresh();
  clear();
  if (header_win)
    delwin(header_win);
  if (cpu_win)
    delwin(cpu_win);
  if (mem_win)
    delwin(mem_win);
  if (proc_win)
    delwin(proc_win);
  int screen_width, screen_height;
  getmaxyx(stdscr, screen_height, screen_width);
  int dummy_cpu_rows = 4;
  int cpu_win_height = dummy_cpu_rows + 2;
  int proc_win_height =
      screen_height - HEADER_HEIGHT - cpu_win_height - MEM_PANEL_HEIGHT;
  header_win = newwin(HEADER_HEIGHT, screen_width, 0, 0);
  cpu_win = newwin(cpu_win_height, screen_width, HEADER_HEIGHT, 0);
  mem_win =
      newwin(MEM_PANEL_HEIGHT, screen_width, HEADER_HEIGHT + cpu_win_height, 0);
  proc_win = newwin(proc_win_height, screen_width,
                    HEADER_HEIGHT + cpu_win_height + MEM_PANEL_HEIGHT, 0);
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
  int drawable_height = proc_win_height - 3;
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

static void format_memory_unit(char *buf, size_t buf_size, long kb) {
  double val = (double)kb;
  if (val < 1024.0) {
    snprintf(buf, buf_size, "%ldK", kb);
  } else if (val < 1024.0 * 1024.0) {
    snprintf(buf, buf_size, "%.1fM", val / 1024.0);
  } else if (val < 1024.0 * 1024.0 * 1024.0) {
    snprintf(buf, buf_size, "%.1fG", val / (1024.0 * 1024.0));
  } else if (val < 1024.0 * 1024.0 * 1024.0 * 1024.0) {
    snprintf(buf, buf_size, "%.1fT", val / (1024.0 * 1024.0 * 1024.0));
  } else {
    snprintf(buf, buf_size, "%.1fP", val / (1024.0 * 1024.0 * 1024.0 * 1024.0));
  }
}

void draw_header(void) {
  werase(header_win);
  wbkgd(header_win, COLOR_PAIR(HEADER_PAIR));
  mvwprintw(header_win, 0, 1, "Pulse - Sort: (c)pu/(p)id | (q)uit");
}

void draw_panel_border(WINDOW *win, const char *title) {
  wattron(win, COLOR_PAIR(PANEL_BORDER_PAIR));
  box(win, 0, 0);
  mvwprintw(win, 0, 2, " %s ", title);
  wattroff(win, COLOR_PAIR(PANEL_BORDER_PAIR));
}

void draw_cpu_panel(const double *cpu_usage, int num_total_cpu_entries) {
  werase(cpu_win);
  draw_panel_border(cpu_win, "CPU");
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
  werase(mem_win);
  draw_panel_border(mem_win, "Memory");

  unsigned long mem_used =
      mem_info->memTotal > 0 ? mem_info->memTotal - mem_info->memAvailable : 0;
  unsigned long swap_used =
      mem_info->swapTotal > 0 ? mem_info->swapTotal - mem_info->swapFree : 0;

  char mem_used_str[16], mem_total_str[16], swap_used_str[16],
      swap_total_str[16];
  format_memory_unit(mem_used_str, sizeof(mem_used_str), mem_used);
  format_memory_unit(mem_total_str, sizeof(mem_total_str), mem_info->memTotal);
  format_memory_unit(swap_used_str, sizeof(swap_used_str), swap_used);
  format_memory_unit(swap_total_str, sizeof(swap_total_str),
                     mem_info->swapTotal);

  char mem_display_str[64], swap_display_str[64];
  snprintf(mem_display_str, sizeof(mem_display_str), "Mem: %s/%s", mem_used_str,
           mem_total_str);
  snprintf(swap_display_str, sizeof(swap_display_str), "Swap: %s/%s",
           swap_used_str, swap_total_str);

  mvwprintw(mem_win, 1, 2, mem_display_str);
  mvwprintw(mem_win, 1, 4 + strlen(mem_display_str), swap_display_str);
}

void draw_process_panel(const ProcessInfo *processes, int num_processes) {
  werase(proc_win);
  draw_panel_border(proc_win, "Processes");
  int width = getmaxx(proc_win);
  int height = getmaxy(proc_win);
  int drawable_height = height - 3;
  if (drawable_height < 1)
    return;

  wattron(proc_win, COLOR_PAIR(PROC_HEADER_PAIR));
  mvwprintw(proc_win, 1, 1, "%-6s %-20s %-5s %-6s %-8s %-8s", "PID", "COMMAND",
            "S", "CPU%", "VIRT", "RES");
  for (int x = 62; x < width - 1; ++x)
    mvwaddch(proc_win, 1, x, ' ');
  wattroff(proc_win, COLOR_PAIR(PROC_HEADER_PAIR));

  for (int i = 0; i < drawable_height; ++i) {
    int proc_index = scroll_offset + i;
    if (proc_index >= num_processes)
      break;

    const ProcessInfo *p = &processes[proc_index];
    char cmd[21], virt_str[16], res_str[16];
    snprintf(cmd, sizeof(cmd), "%.20s", p->stats.comm);
    format_memory_unit(virt_str, sizeof(virt_str), p->stats.vsize / 1024);
    format_memory_unit(res_str, sizeof(res_str), p->stats.rss * 4);

    mvwprintw(proc_win, i + 2, 1, "%-6d %-20s %-5c %-6.1f %-8s %-8s",
              p->stats.pid, cmd, p->stats.state, p->cpu_percent, virt_str,
              res_str);
  }

  int max_scroll = num_processes - drawable_height;
  if (max_scroll > 0) {
    double scroll_percent = (double)scroll_offset / max_scroll;
    int thumb_y = 2 + (int)(scroll_percent * (drawable_height - 1));
    wattron(proc_win, A_DIM);
    mvwvline(proc_win, 2, width - 2, ACS_VLINE, drawable_height);
    wattroff(proc_win, A_DIM);
    wattron(proc_win, COLOR_PAIR(SCROLL_THUMB_PAIR));
    mvwaddch(proc_win, thumb_y, width - 2, ACS_BLOCK);
    wattroff(proc_win, COLOR_PAIR(SCROLL_THUMB_PAIR));
  }
}
