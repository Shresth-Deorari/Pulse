#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <pthread.h>

#include "../include/parser.h"
#include "../include/calculate.h"
#include "../include/ui.h"

#define INITIAL_BUFFER_SIZE 4096
#define MAX_CPU_ENTRIES 33
#define MAX_PROCESSES 4096 // Increased for safety

typedef struct {
    pidStats* items;
    int count;
} ProcessList;

typedef struct {
    double cpu_usage[MAX_CPU_ENTRIES];
    memStats mem_info;
    ProcessInfo processed_list[MAX_PROCESSES];
    int num_total_cpu_entries;
    int num_processes;
} SharedData;

static SharedData shared_data;
static pthread_mutex_t data_mutex;
static volatile int running = 1;

char *read_file_dynamically(const char *path);
int get_all_processes(pidStats* buffer, int capacity);
void* data_collector_thread(void* arg);
int compare_pids(const void* a, const void* b);

int main() {
    pthread_t data_thread_id;

    if (pthread_mutex_init(&data_mutex, NULL) != 0) { return 1; }
    if (pthread_create(&data_thread_id, NULL, data_collector_thread, NULL) != 0) { return 1; }

    ui_init();

    while (running) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            running = 0;
            continue;
        } else if (ch == KEY_RESIZE) {
            ui_resize();
        } else if (ch != ERR) {
            ui_handle_input(ch, shared_data.num_processes);
        }

        SharedData local_data_copy;
        pthread_mutex_lock(&data_mutex);
        local_data_copy = shared_data;
        pthread_mutex_unlock(&data_mutex);

        ui_draw(local_data_copy.cpu_usage, &local_data_copy.mem_info, 
                local_data_copy.num_total_cpu_entries, 
                local_data_copy.processed_list, local_data_copy.num_processes);
        
        usleep(33000); // ~30 FPS
    }

    pthread_join(data_thread_id, NULL);
    pthread_mutex_destroy(&data_mutex);
    ui_cleanup();
    return 0;
}

void* data_collector_thread(void* arg) {
    (void)arg;
    
    cpuStat prevCpuStats[MAX_CPU_ENTRIES] = {0}, currCpuStats[MAX_CPU_ENTRIES] = {0};
    pidStats prev_procs_buffer[MAX_PROCESSES];
    ProcessList prev_procs = { .items = prev_procs_buffer };
    int num_cpu_entries = 0;

    char *initial_cpu_data = read_file_dynamically("/proc/stat");
    if (initial_cpu_data) {
        num_cpu_entries = cpuParser(initial_cpu_data, prevCpuStats, MAX_CPU_ENTRIES);
        free(initial_cpu_data);
    }
    prev_procs.count = get_all_processes(prev_procs.items, MAX_PROCESSES);

    while (running) {
        SharedData current_data;
        pidStats curr_procs_buffer[MAX_PROCESSES];
        ProcessList curr_procs = { .items = curr_procs_buffer };

        current_data.num_total_cpu_entries = num_cpu_entries;

        char* cpu_data = read_file_dynamically("/proc/stat");
        char* mem_data = read_file_dynamically("/proc/meminfo");
        curr_procs.count = get_all_processes(curr_procs.items, MAX_PROCESSES);

        if (cpu_data) {
            cpuParser(cpu_data, currCpuStats, MAX_CPU_ENTRIES);
            cpuUsage(prevCpuStats, currCpuStats, current_data.cpu_usage, num_cpu_entries);
        }
        if (mem_data) {
            memParser(mem_data, &current_data.mem_info);
            free(mem_data);
        }

        long long total_cpu_time_delta = 0;
        if (cpu_data) {
            unsigned long long prev_total = prevCpuStats[0].user + prevCpuStats[0].nice + prevCpuStats[0].system + prevCpuStats[0].idle + prevCpuStats[0].iowait + prevCpuStats[0].irq + prevCpuStats[0].softirq;
            unsigned long long curr_total = currCpuStats[0].user + currCpuStats[0].nice + currCpuStats[0].system + currCpuStats[0].idle + currCpuStats[0].iowait + currCpuStats[0].irq + currCpuStats[0].softirq;
            total_cpu_time_delta = curr_total - prev_total;
        }

        qsort(prev_procs.items, prev_procs.count, sizeof(pidStats), compare_pids);

        for (int i = 0; i < curr_procs.count; ++i) {
            current_data.processed_list[i].stats = curr_procs.items[i];
            current_data.processed_list[i].cpu_percent = 0.0;
            
            pidStats* prev_stat = bsearch(&curr_procs.items[i], prev_procs.items, prev_procs.count, sizeof(pidStats), compare_pids);

            if (prev_stat) {
                unsigned long long proc_time_delta = (curr_procs.items[i].utime + curr_procs.items[i].stime) - (prev_stat->utime + prev_stat->stime);
                if (total_cpu_time_delta > 0) {
                    int num_cores = num_cpu_entries > 1 ? num_cpu_entries - 1 : 1;
                    current_data.processed_list[i].cpu_percent = 100.0 * (double)proc_time_delta * num_cores / (double)total_cpu_time_delta;
                }
            }

            if (current_data.mem_info.memTotal > 0) {
                current_data.processed_list[i].mem_percent = 100.0 * (double)(current_data.processed_list[i].stats.rss * 4) / (double)current_data.mem_info.memTotal;
            }
        }
        current_data.num_processes = curr_procs.count;
        
        pthread_mutex_lock(&data_mutex);
        shared_data = current_data;
        pthread_mutex_unlock(&data_mutex);

        if (cpu_data) {
            updateCpuState(prevCpuStats, currCpuStats, num_cpu_entries);
            free(cpu_data);
        }
        
        prev_procs.count = curr_procs.count;
        memcpy(prev_procs.items, curr_procs.items, curr_procs.count * sizeof(pidStats));
        
        sleep(1);
    }
    
    return NULL;
}

int compare_pids(const void* a, const void* b) {
    const pidStats* p1 = (const pidStats*)a;
    const pidStats* p2 = (const pidStats*)b;
    return (p1->pid - p2->pid);
}

int get_all_processes(pidStats* buffer, int capacity) {
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) return 0;

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL && count < capacity) {
        if (!isdigit(entry->d_name[0])) continue;
        
        char path[512];
        snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
        char *file_contents = read_file_dynamically(path);
        if (file_contents) {
            if (pidParser(file_contents, &buffer[count])) count++;
            free(file_contents);
        }
    }
    closedir(proc_dir);
    return count;
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
