#ifndef CALCULATE
#define CALCULATE

#include "parser.h"

void updateCpuState(cpuStat *prevCpuStats, cpuStat *currentCpuStats);

void cpuUsage(cpuStat *prevCpuStats, cpuStat *currentCpuStats,
                 double *usage);

#endif
