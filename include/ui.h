#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include "parser.h"

void ui_init(void);

void ui_cleanup(void);

void ui_handle_input(int ch);

void ui_draw(const double* cpu_usage, const memStats* mem_info, int num_cores);

#endif
