// src/ui.c
#include "../include/ui.h"
#include <string.h>
#include <ncurses.h>

static WINDOW *header_win;
static WINDOW *cpu_win;
static WINDOW *mem_win;

#define HEADER_HEIGHT 1
#define CPU_WIN_HEIGHT 6 
#define MEM_WIN_HEIGHT 6

void draw_header(void);
void draw_panel_border(WINDOW *win, const char* title);
void draw_meter(WINDOW *win, int y, int x, int width, double percent);
void draw_cpu_panel(const double* cpu_usage, int num_cores);
void draw_mem_panel(const memStats* mem_info);


void ui_init(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
    }
    
    int screen_width, screen_height;
    getmaxyx(stdscr, screen_height, screen_width);

    header_win = newwin(HEADER_HEIGHT, screen_width, 0, 0);
    cpu_win = newwin(CPU_WIN_HEIGHT, screen_width / 2, HEADER_HEIGHT + 1, 0);
    mem_win = newwin(MEM_WIN_HEIGHT, screen_width / 2, HEADER_HEIGHT + 1, screen_width / 2);
}

void ui_cleanup(void) {
    delwin(header_win);
    delwin(cpu_win);
    delwin(mem_win);
    endwin();
}

void ui_handle_input(int ch) {
    switch(ch) {
        case KEY_UP:
            break;
        case KEY_DOWN:
            break;
    }
}

void ui_draw(const double* cpu_usage, const memStats* mem_info, int num_cores) {
    werase(stdscr);
    werase(header_win);
    werase(cpu_win);
    werase(mem_win);

    draw_header();
    draw_cpu_panel(cpu_usage, num_cores);
    draw_mem_panel(mem_info);

    wnoutrefresh(stdscr);
    wnoutrefresh(header_win);
    wnoutrefresh(cpu_win);
    wnoutrefresh(mem_win);
    doupdate();
}


void draw_header(void) {
    int width = getmaxx(header_win);
    wbkgd(header_win, COLOR_PAIR(1));
    mvwprintw(header_win, 0, (width - 24) / 2, "Pulse - Resource Monitor");
    wattron(header_win, A_BOLD);
    mvwprintw(header_win, 0, width - 10, "(q to quit)");
    wattroff(header_win, A_BOLD);
}

void draw_panel_border(WINDOW *win, const char* title) {
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", title);
}

void draw_meter(WINDOW *win, int y, int x, int width, double percent) {
    int filled_width = (int)(percent / 100.0 * width);
    
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, y, x, "%*s", filled_width, "");
    wattroff(win, COLOR_PAIR(2));
    mvwprintw(win, y, x + filled_width, "%*s", width - filled_width, "");

    char percent_str[10];
    snprintf(percent_str, sizeof(percent_str), "%.1f%%", percent);
    mvwprintw(win, y, x + (width - strlen(percent_str)) / 2, percent_str);
}

void draw_cpu_panel(const double* cpu_usage, int num_cores) {
    draw_panel_border(cpu_win, "CPU Usage");
    int panel_width = getmaxx(cpu_win);
    int meter_width = panel_width - 12;

    mvwprintw(cpu_win, 2, 2, "Aggr:");
    wattron(cpu_win, A_REVERSE);
    draw_meter(cpu_win, 2, 8, meter_width, cpu_usage[0]);
    wattroff(cpu_win, A_REVERSE);

    for (int i = 0; i < num_cores && i < 2; ++i) {
        char core_label[10];
        snprintf(core_label, sizeof(core_label), "CPU%d:", i);
        mvwprintw(cpu_win, 3 + i, 2, "%s", core_label);
        draw_meter(cpu_win, 3 + i, 8, meter_width, cpu_usage[i + 1]);
    }
}

void draw_mem_panel(const memStats* mem_info) {
    draw_panel_border(mem_win, "Memory");
    int panel_width = getmaxx(mem_win);
    int meter_width = panel_width - 4;

    double mem_percent = 0.0;
    if (mem_info->memTotal > 0) {
        mem_percent = 100.0 * (mem_info->memTotal - mem_info->memAvailable) / mem_info->memTotal;
    }
    mvwprintw(mem_win, 1, 2, "RAM:");
    draw_meter(mem_win, 1, 8, meter_width - 6, mem_percent);

    double swap_percent = 0.0;
    if (mem_info->swapTotal > 0) {
        swap_percent = 100.0 * (mem_info->swapTotal - mem_info->swapFree) / mem_info->swapTotal;
    }
    mvwprintw(mem_win, 2, 2, "Swp:");
    draw_meter(mem_win, 2, 8, meter_width - 6, swap_percent);

    mvwprintw(mem_win, 4, 2, "Mem Total: %-8lu KB", mem_info->memTotal);
    mvwprintw(mem_win, 4, panel_width / 2, "Swap Total: %-8lu KB", mem_info->swapTotal);
    mvwprintw(mem_win, 5, 2, "Mem Avail: %-8lu KB", mem_info->memAvailable);
    mvwprintw(mem_win, 5, panel_width / 2, "Swap Free:  %-8lu KB", mem_info->swapFree);
}
