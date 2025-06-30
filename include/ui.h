#ifndef UI_H
#define UI_H

#include "parser.h"
#include <ncurses.h>

typedef struct {
  pidStats stats;
  double cpu_percent;
  double mem_percent;
} ProcessInfo;

void ui_init(void);

void ui_cleanup(void);

void ui_handle_input(int ch, int num_processes);

void ui_draw(const double *cpu_usage, const memStats *mem_info, int num_cores,
             const ProcessInfo *processes, int num_processes);
void ui_resize(void);

#endif
