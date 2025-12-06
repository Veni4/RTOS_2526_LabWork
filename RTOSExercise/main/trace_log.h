#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdint.h>

    /*
    * current tick count
    * time stamp (more fine-grained than ticks)
    * the queue handle,
    * number of ticks that the job will wait for
    * identifier of the task that calls the function
    */

struct trace_log_entry {
    TickType_t    tick;
    uint64_t      timestamp_us;
    QueueHandle_t queue;
    TickType_t    block_time;
    TaskHandle_t  task;
    char          log_type;
};

#define TRACE_LOG_BUFFER_SIZE 512
extern struct trace_log_entry trace_log_buffer[TRACE_LOG_BUFFER_SIZE];
extern volatile uint32_t trace_log_head;

void add_trace_log_entry(unsigned long tick, unsigned long long timestamp_us, void* queue, unsigned long block_time, void* task, char log_type);
void print_trace_logs(void);
void flush_logs_to_file(void);

#ifdef __cplusplus
}
#endif