#ifndef PARSER
#define PARSER

typedef struct {
  unsigned long memTotal;
  unsigned long memAvailable;
  unsigned long swapTotal;
  unsigned long swapFree;
} memStats;

typedef struct {
  unsigned long user;
  unsigned long nice;
  unsigned long system;
  unsigned long idle;
  unsigned long iowait;
  unsigned long irq;
  unsigned long softirq;
} cpuStat;

typedef struct {
  int pid;
  char comm[256], state;
  unsigned long utime, stime;
  long vsize, rss;
} pidStats;

void memParser(char *input, memStats * stats);

void cpuParser(char *input, cpuStat * stats);

void pidParser(char *input, pidStats * stats);

#endif
