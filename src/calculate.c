#include "../include/calculate.h"
#include "../include/parser.h"
#include <string.h>

void updateCpuState(cpuStat *prevCpuStats, cpuStat *currentCpuStats,
                    int num_entries) {
  memcpy(prevCpuStats, currentCpuStats, sizeof(cpuStat) * num_entries);
}

void cpuUsage(cpuStat *prevCpuStats, cpuStat *currentCpuStats, double *usage,
              int num_entries) {
  for (int i = 0; i < num_entries; i++) {
    unsigned long long prevIdle = prevCpuStats[i].idle + prevCpuStats[i].iowait;
    unsigned long long idle = currentCpuStats[i].idle + currentCpuStats[i].iowait;

    unsigned long long prevNonIdle =
        prevCpuStats[i].user + prevCpuStats[i].nice + prevCpuStats[i].system +
        prevCpuStats[i].irq + prevCpuStats[i].softirq;
    unsigned long long nonIdle =
        currentCpuStats[i].user + currentCpuStats[i].nice +
        currentCpuStats[i].system + currentCpuStats[i].irq +
        currentCpuStats[i].softirq;

    unsigned long long prevTotal = prevIdle + prevNonIdle;
    unsigned long long total = idle + nonIdle;

    unsigned long long totald = total - prevTotal;
    unsigned long long idled = idle - prevIdle;
    
    if (totald > 0) {
      usage[i] = ((double)(totald - idled) / (double)totald) * 100.0;
    } else {
      usage[i] = 0.0;
    }
  }
}
