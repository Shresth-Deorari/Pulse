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
  /*printf("\nTotal Memory: %lu\nFree Memory: %lu\nTotal Swap Space: %lu\nFree "
         "Swap Space: %lu\n",
         stats->memTotal, stats->memAvailable, stats->swapTotal,
         stats->swapFree); */
}

int cpuParser(char *input, cpuStat *cpuStatsPointer, int max_entries) {
  char *line = input;
  char *next_line = NULL;
  int count = 0;

  while (line && *line != '\0' && count < max_entries) {
    next_line = strchr(line, '\n');
    if (next_line) {
      *next_line = '\0';
    }

    if (strncmp(line, "cpu", 3) == 0) {
      cpuStat *stats = &cpuStatsPointer[count];
      int core_id;

      if (sscanf(line, " cpu %lu %lu %lu %lu %lu %lu %lu", &stats->user,
                 &stats->nice, &stats->system, &stats->idle, &stats->iowait,
                 &stats->irq, &stats->softirq) == 7) {
        count++;
      } else if (sscanf(line, " cpu%d %lu %lu %lu %lu %lu %lu %lu", &core_id,
                        &stats->user, &stats->nice, &stats->system,
                        &stats->idle, &stats->iowait, &stats->irq,
                        &stats->softirq) == 8) {
        count++;
      }
    }

    if (next_line) {
      line = next_line + 1;
    } else {
      break;
    }
  }
  return count;
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
  /*printf("%d \n%s \n%c \n%lu \n%lu \n%ld \n%ld\n", stats->pid, stats->comm,
         stats->state, stats->utime, stats->stime, stats->vsize, stats->rss);*/
}
