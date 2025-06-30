#ifndef CALCULATE
#define CALCULATE

#include "parser.h"

void updateCpuState(cpuStat *prevCpuStats, cpuStat *currentCpuStats,
                    int num_entries);

void cpuUsage(cpuStat *prevCpuStats, cpuStat *currentCpuStats, double *usage,
              int num_entries);

#endif
