#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include "../include/parser.h"
#include "../include/calculate.h"
#include "../include/ui.h"

#define INITIAL_BUFFER_SIZE 4096
#define NUM_CPU_STATS 17

char *read_file_dynamically(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }

    size_t buffer_size = INITIAL_BUFFER_SIZE + 1;
    size_t total_read = 0;
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        close(fd);
        return NULL;
    }
    while (1) {
        ssize_t bytes_read = read(fd, buffer + total_read, buffer_size - 1 - total_read);
        if (bytes_read > 0) {
            total_read += bytes_read;
        } else if (bytes_read == 0) {
            break;
        } else {
            free(buffer);
            close(fd);
            return NULL;
        }

        if (total_read == buffer_size - 1) {
            buffer_size *= 2;
            char *new_buffer = realloc(buffer, buffer_size);
            if (new_buffer == NULL) {
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


int main() {
    cpuStat prevCpuStats[NUM_CPU_STATS] = {0};
    cpuStat currCpuStats[NUM_CPU_STATS] = {0};
    memStats mem_info = {0};
    double cpu_usage[NUM_CPU_STATS] = {0.0};

    char *initial_cpu_data = read_file_dynamically("/proc/stat");
    if (initial_cpu_data) {
        cpuParser(initial_cpu_data, prevCpuStats);
        free(initial_cpu_data);
    }
    
    int num_cores = 0;
    for (int i = 1; i < NUM_CPU_STATS; ++i) {
        if (prevCpuStats[i].user > 0 || prevCpuStats[i].system > 0 || prevCpuStats[i].idle > 0) {
            num_cores++;
        } else {
            break;
        }
    }
    if (num_cores == 0) num_cores = 1; // Failsafe


    ui_init();

    int running = 1;
    while (running) {
        char* cpu_data = read_file_dynamically("/proc/stat");
        char* mem_data = read_file_dynamically("/proc/meminfo");
        
        if (cpu_data) {
            cpuParser(cpu_data, currCpuStats);
            cpuUsage(prevCpuStats, currCpuStats, cpu_usage);
            updateCpuState(prevCpuStats, currCpuStats);
            free(cpu_data);
        }

        if (mem_data) {
            memParser(mem_data, &mem_info);
            free(mem_data);
        }
        
        ui_draw(cpu_usage, &mem_info, num_cores);

        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            running = 0;
        } else if (ch != ERR) {
            ui_handle_input(ch);
        }

        // --- TIMING ---
        usleep(500000); // Sleep for 0.5 seconds
    }

    // --- CLEANUP ---
    ui_cleanup();
    return 0;
}
