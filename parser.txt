#include "../include/parser.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>

void memParser(char *input, memStats *stats) {
  char *token;
  char key[64];
  unsigned long value;
  while ((token = strtok_r(input, "\n", &input))) {
    if (sscanf(token, "%63s %lu", key, &value) == 2) {
      if (strcmp(key, "MemTotal:") == 0)
        stats->memTotal = value;
      else if (strcmp(key, "MemAvailable:") == 0)
        stats->memAvailable = value;
      else if (strcmp(key, "SwapTotal:") == 0)
        stats->swapTotal = value;
      else if (strcmp(key, "SwapFree:") == 0)
        stats->swapFree = value;
    }
  }
  printf("\nTotal Memory: %lu\nFree Memory: %lu\nTotal Swap Space: %lu\nFree "
         "Swap Space: %lu\n",
         stats->memTotal, stats->memAvailable, stats->swapTotal,
         stats->swapFree);
}

void cpuParser(char *input, cpuStat *cpuStatsPointer) {
  cpuStat *stats = cpuStatsPointer;
  char *token;
  token = strtok_r(input, "\n", &input);
  int readCount =
      sscanf(token, " cpu %lu %lu %lu %lu %lu %lu %lu %*lu %*lu %*lu",
             &stats->user, &stats->nice, &stats->system, &stats->idle,
             &stats->iowait, &stats->irq, &stats->softirq);
  if (readCount < 7) {
    fprintf(stderr, "Failed to parse cpu stats\n");
    return;
  }
  printf("cpu %lu %lu %lu %lu %lu %lu %lu\n", stats->user, stats->nice,
         stats->system, stats->idle, stats->iowait, stats->irq, stats->softirq);
  stats++;
  while ((token = strtok_r(input, "\n", &input))) {
    int core;
    readCount =
        sscanf(token, " cpu%d %lu %lu %lu %lu %lu %lu %lu %*lu %*lu %*lu",
               &core, &stats->user, &stats->nice, &stats->system, &stats->idle,
               &stats->iowait, &stats->irq, &stats->softirq);
    if (readCount < 8) {
      fprintf(stderr, "Failed to parse cpu stats\n");
      return;
    }
    printf("cpu%d %lu %lu %lu %lu %lu %lu %lu\n", core, stats->user,
           stats->nice, stats->system, stats->idle, stats->iowait, stats->irq,
           stats->softirq);
    stats++;
    if (core == 15)
      break;
  }
}

void pidParser(char *input, pidStats *stats) {
  int readCount = sscanf(input,
                         "%d (%255[^)]) %c %*d %*d %*d %*d %*d %*d %*d %*d %*d "
                         "%lu %lu %*ld %*ld %*ld %*ld %*ld %*ld %*llu %lu %ld",
                         &stats->pid, stats->comm, &stats->state, &stats->utime,
                         &stats->stime, &stats->vsize, &stats->rss);
  if (readCount < 7) {
    fprintf(stderr, "Failed to parse file completly\n");
    return;
  }
  printf("%d \n%s \n%c \n%lu \n%lu \n%ld \n%ld\n", stats->pid, stats->comm,
         stats->state, stats->utime, stats->stime, stats->vsize, stats->rss);
}
