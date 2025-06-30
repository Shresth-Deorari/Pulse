// src/main.c
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

// FIX: Increase array size to prevent stack smashing.
// A 16-core CPU has 17 CPU entries (1 aggr + 16 cores).
// A 32-core CPU has 33. We'll set a generous max.
#define MAX_CPU_ENTRIES 33 

// Your existing helper function, unchanged
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

        if (total_read >= buffer_size - 1) {
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
    // --- INITIALIZATION ---
    // FIX: Use the new, larger size for all CPU-related arrays.
    cpuStat prevCpuStats[MAX_CPU_ENTRIES] = {0};
    cpuStat currCpuStats[MAX_CPU_ENTRIES] = {0};
    memStats mem_info = {0};
    double cpu_usage[MAX_CPU_ENTRIES] = {0.0};

    // Get initial CPU state to calculate usage on the first real loop
    char *initial_cpu_data = read_file_dynamically("/proc/stat");
    int num_total_cpu_entries = 0; // This will be number of cores + 1
    if (initial_cpu_data) {
        // FIX: The parser now returns the number of CPU entries it found.
        num_total_cpu_entries = cpuParser(initial_cpu_data, prevCpuStats, MAX_CPU_ENTRIES);
        free(initial_cpu_data);
    }
    
    // If parsing failed or returned 0, we can't continue.
    if (num_total_cpu_entries == 0) {
        fprintf(stderr, "Could not read or parse /proc/stat. Exiting.\n");
        return 1;
    }

    ui_init();

    // --- MAIN LOOP ---
    int running = 1;
    while (running) {
        // --- DATA FETCHING AND PROCESSING ---
        char* cpu_data = read_file_dynamically("/proc/stat");
        char* mem_data = read_file_dynamically("/proc/meminfo");
        
        if (cpu_data) {
            cpuParser(cpu_data, currCpuStats, MAX_CPU_ENTRIES);
            cpuUsage(prevCpuStats, currCpuStats, cpu_usage, num_total_cpu_entries);
            updateCpuState(prevCpuStats, currCpuStats, num_total_cpu_entries);
            free(cpu_data);
        }

        if (mem_data) {
            memParser(mem_data, &mem_info);
            free(mem_data);
        }
        
        // --- DRAWING ---
        // Pass the actual number of CPU entries to the draw function.
        ui_draw(cpu_usage, &mem_info, num_total_cpu_entries);

        // --- INPUT HANDLING ---
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            running = 0;
        } else if (ch != ERR) {
            ui_handle_input(ch);
        }

        // --- TIMING ---
        // FIX: Shorten sleep time for better responsiveness to input.
        usleep(250000); // Sleep for 0.25 seconds
    }

    // --- CLEANUP ---
    ui_cleanup();
    return 0;
}
