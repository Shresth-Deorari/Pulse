// src/main.c
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/calculate.h"
#include "../include/parser.h"
#include "../include/ui.h"

#define INITIAL_BUFFER_SIZE 4096
#define MAX_CPU_ENTRIES 33
#define MAX_PROCESSES 1024

typedef struct {
  pidStats *items;
  int count;
} ProcessList;

char *read_file_dynamically(const char *path);
ProcessList get_all_processes(void);

int main() {
  // --- Initialization ---
  cpuStat prevCpuStats[MAX_CPU_ENTRIES] = {0},
          currCpuStats[MAX_CPU_ENTRIES] = {0};
  memStats mem_info = {0};
  double cpu_usage[MAX_CPU_ENTRIES] = {0.0};
  int num_total_cpu_entries = 0;
  ProcessList prev_procs = {.items = NULL, .count = 0};
  ProcessList curr_procs = {.items = NULL, .count = 0};
  ProcessInfo processed_list[MAX_PROCESSES];

  char *initial_cpu_data = read_file_dynamically("/proc/stat");
  if (initial_cpu_data) {
    num_total_cpu_entries =
        cpuParser(initial_cpu_data, prevCpuStats, MAX_CPU_ENTRIES);
    free(initial_cpu_data);
  }
  if (num_total_cpu_entries == 0) {
    fprintf(stderr, "Could not read or parse /proc/stat. Exiting.\n");
    return 1;
  }
  prev_procs = get_all_processes();

  ui_init();

  // --- Main Loop ---
  int running = 1;
  while (running) {
    // ====================================================================
    // FIX: Handle input FIRST to ensure the application is always responsive.
    // ====================================================================
    int ch = getch();
    if (ch == 'q' || ch == 'Q') {
      running = 0;
      continue; // Skip the rest of the loop and exit gracefully
    } else if (ch != ERR) {
      ui_handle_input(ch, curr_procs.count);
    }

    // --- Now, do all the data fetching and processing ---
    char *cpu_data = read_file_dynamically("/proc/stat");
    char *mem_data = read_file_dynamically("/proc/meminfo");

    if (cpu_data) {
      cpuParser(cpu_data, currCpuStats, MAX_CPU_ENTRIES);
      cpuUsage(prevCpuStats, currCpuStats, cpu_usage, num_total_cpu_entries);
    }
    if (mem_data) {
      memParser(mem_data, &mem_info);
      free(mem_data);
    }

    curr_procs = get_all_processes();

    long long total_cpu_time_delta = 0;
    if (cpu_data) {
      unsigned long long prev_total =
          prevCpuStats[0].user + prevCpuStats[0].nice + prevCpuStats[0].system +
          prevCpuStats[0].idle + prevCpuStats[0].iowait + prevCpuStats[0].irq +
          prevCpuStats[0].softirq;
      unsigned long long curr_total =
          currCpuStats[0].user + currCpuStats[0].nice + currCpuStats[0].system +
          currCpuStats[0].idle + currCpuStats[0].iowait + currCpuStats[0].irq +
          currCpuStats[0].softirq;
      total_cpu_time_delta = curr_total - prev_total;
    }

    for (int i = 0; i < curr_procs.count; ++i) {
      processed_list[i].stats = curr_procs.items[i];
      processed_list[i].cpu_percent = 0.0;

      for (int j = 0; j < prev_procs.count; ++j) {
        if (curr_procs.items[i].pid == prev_procs.items[j].pid) {
          unsigned long long proc_time_delta =
              (curr_procs.items[i].utime + curr_procs.items[i].stime) -
              (prev_procs.items[j].utime + prev_procs.items[j].stime);
          if (total_cpu_time_delta > 0) {
            processed_list[i].cpu_percent =
                100.0 * (double)proc_time_delta / (double)total_cpu_time_delta;
          }
          break;
        }
      }
      if (mem_info.memTotal > 0) {
        processed_list[i].mem_percent =
            100.0 * (double)(processed_list[i].stats.rss * 4) /
            (double)mem_info.memTotal;
      }
    }

    // --- Drawing ---
    ui_draw(cpu_usage, &mem_info, num_total_cpu_entries, processed_list,
            curr_procs.count);

    // --- State Update & Cleanup ---
    if (cpu_data) {
      updateCpuState(prevCpuStats, currCpuStats, num_total_cpu_entries);
      free(cpu_data);
    }
    free(prev_procs.items);
    prev_procs = curr_procs;

    // FIX: Reduced sleep time for a smoother, more responsive feel
    usleep(250000); // 0.25 sec refresh rate
  }

  // --- Final Cleanup ---
  free(prev_procs.items); // Free the last list
  ui_cleanup();
  return 0;
}

ProcessList get_all_processes() {
  DIR *proc_dir = opendir("/proc");
  if (!proc_dir)
    return (ProcessList){.items = NULL, .count = 0};

  ProcessList list = {.items = malloc(sizeof(pidStats) * MAX_PROCESSES),
                      .count = 0};
  if (!list.items) {
    closedir(proc_dir);
    return (ProcessList){.items = NULL, .count = 0};
  }

  struct dirent *entry;
  while ((entry = readdir(proc_dir)) != NULL && list.count < MAX_PROCESSES) {
    if (!isdigit(entry->d_name[0]))
      continue;
    char path[512];
    snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
    char *file_contents = read_file_dynamically(path);
    if (file_contents) {
      if (pidParser(file_contents, &list.items[list.count]))
        list.count++;
      free(file_contents);
    }
  }
  closedir(proc_dir);
  return list;
}

char *read_file_dynamically(const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0)
    return NULL;
  size_t buffer_size = INITIAL_BUFFER_SIZE;
  char *buffer = malloc(buffer_size);
  if (!buffer) {
    close(fd);
    return NULL;
  }
  size_t total_read = 0;
  while (1) {
    ssize_t bytes_read =
        read(fd, buffer + total_read, buffer_size - total_read - 1);
    if (bytes_read > 0) {
      total_read += bytes_read;
    } else if (bytes_read == 0) {
      break;
    } else {
      free(buffer);
      close(fd);
      return NULL;
    }
    if (total_read >= buffer_size - 1) {
      buffer_size *= 2;
      char *new_buffer = realloc(buffer, buffer_size);
      if (!new_buffer) {
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
