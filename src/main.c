#include "../include/calculate.h"
#include "../include/parser.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ui.h"

#define INITIAL_BUFFER_SIZE 4096

void displayCpuUsage(double *usage) {
  for (int i = 0; i < 16; i++) {
    if (!i)
      printf("Aggregate CPU \% Used: %lf\n", usage[i]);
    else
      printf("CPU%d \% Usage: %lf\n", i-1, usage[i]);
  }
}

char *read_file_dynamically(const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return NULL;
  }

  size_t buffer_size = INITIAL_BUFFER_SIZE + 1;
  size_t total_read = 0;
  char *buffer = malloc(buffer_size);
  if (buffer == NULL) {
    perror("malloc");
    close(fd);
    return NULL;
  }
  while (1) {
    ssize_t bytes_read =
        read(fd, buffer + total_read, buffer_size - total_read);
    if (bytes_read > 0) {
      total_read += bytes_read;
    } else if (bytes_read == 0) {
      break;
    } else {
      perror("read");
      free(buffer);
      close(fd);
      return NULL;
    }

    if (total_read == buffer_size - 1) {
      buffer_size *= 2;
      char *new_buffer = realloc(buffer, buffer_size);
      if (new_buffer == NULL) {
        perror("realloc");
        free(buffer);
        close(fd);
        return NULL;
      }
      buffer = new_buffer;
    }
  }
  buffer[total_read] = '\0';
  close(fd);
  return buffer;
}

void mainLoop(cpuStat *prevCpuStats, cpuStat *currCpuStats) {
  char statPath[] = "/proc/stat";
  char meminfoPath[] = "/proc/meminfo";
  char *readCPU = read_file_dynamically(statPath);
  char *readMem = read_file_dynamically(meminfoPath);
  DIR *procPath = opendir("/proc");
  if (procPath == NULL) {
    perror("opendir");
    free(readCPU);
    free(readMem);
    exit(1);
  }
  struct dirent *subfilePath;
  while ((subfilePath = readdir(procPath)) != NULL) {
    char *fileName = subfilePath->d_name;
    if (!(0 < (fileName[0] - '0') && (fileName[0] - '0') <= 9))
      continue;
    char pidStatPath[256];
    snprintf(pidStatPath, sizeof(pidStatPath), "/proc/%s/stat", fileName);
    char *fileContents = read_file_dynamically(pidStatPath);
    if (fileContents != NULL) {
      pidStats stats;
      pidParser(fileContents, &stats);
      free(fileContents);
    }
  }
  if (readCPU) {
    double usage[16];
    cpuParser(readCPU, currCpuStats);
    cpuUsage(prevCpuStats, currCpuStats, usage);
    displayCpuUsage(usage);
  }
  if (readMem) {
    memStats stats;
    memParser(readMem, &stats);
  }
  free(readCPU);
  free(readMem);
  closedir(procPath);
}

int main() {
  char statPath[] = "/proc/stat";

  cpuStat prevCpuStats[16] = {0};
  cpuStat currCpuStats[16] = {0};
  char *readCPU = read_file_dynamically(statPath);

  if (readCPU) {
    cpuParser(readCPU, prevCpuStats);
    free(readCPU);
  }
  ui_init();
  while (1) {
    //sleep(1);
    //mainLoop(prevCpuStats, currCpuStats);
    //updateCpuState(prevCpuStats, currCpuStats);
    ui_draw();
    int ch = getch();
    if(ch == 'q')break;
  }
  ui_cleanup();
  return 0;
}
