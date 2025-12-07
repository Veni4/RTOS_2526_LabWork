#include "trace_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <inttypes.h>

struct trace_log_entry trace_log_buffer[TRACE_LOG_BUFFER_SIZE];
volatile uint32_t trace_log_head = 0;

/* Static mutex for thread-safe access to trace log buffer */
static portMUX_TYPE trace_log_mux = portMUX_INITIALIZER_UNLOCKED;


void add_trace_log_entry(unsigned long tick, unsigned long long timestamp_us, void* queue, unsigned long block_time, void* task, char log_type)
{
    struct trace_log_entry e = {
        .tick = (TickType_t)tick,
        .timestamp_us = (uint64_t)timestamp_us,
        .queue = (QueueHandle_t)queue,
        .block_time = (TickType_t)block_time,
        .task = (TaskHandle_t)task,
        .log_type = (char)log_type
    };
    /* Thread-safe: use mutex to atomically update head */
    taskENTER_CRITICAL(&trace_log_mux);
    uint32_t i = trace_log_head % TRACE_LOG_BUFFER_SIZE;
    trace_log_buffer[i] = e;
    trace_log_head++;
    taskEXIT_CRITICAL(&trace_log_mux);
}

void add_trace_log_entry_newtick(unsigned long old_tick, unsigned long long timestamp_us, const char* identifier, unsigned long new_tick, char log_type)
{
    struct trace_log_entry e = {
        .tick = (TickType_t)old_tick,
        .timestamp_us = (uint64_t)timestamp_us,
        .identifier = (const char*)identifier,
        .new_tick = (TickType_t)new_tick,
        .log_type = (char)log_type
    };
    /* Thread-safe: use mutex to atomically update head */
    taskENTER_CRITICAL(&trace_log_mux);
    uint32_t i = trace_log_head % TRACE_LOG_BUFFER_SIZE;
    trace_log_buffer[i] = e;
    trace_log_head++;
    taskEXIT_CRITICAL(&trace_log_mux);
}

void print_trace_logs(void)
{
    /* Capture head atomically to avoid race conditions */
    taskENTER_CRITICAL(&trace_log_mux);
    uint32_t head = trace_log_head;
    taskEXIT_CRITICAL(&trace_log_mux);

    printf("\n========== TRACE LOG (%" PRIu32 " entries) ==========\n", head);

    /* Calculate how many entries to show (handle wrap-around) */
    uint32_t entries_to_show = head;
    if (head > TRACE_LOG_BUFFER_SIZE) {
        entries_to_show = TRACE_LOG_BUFFER_SIZE;
    }

    for (uint32_t i = 0; i < entries_to_show; i++) {
        uint32_t idx = (head - entries_to_show + i) % TRACE_LOG_BUFFER_SIZE;
        struct trace_log_entry *e = &trace_log_buffer[idx];
        const char* task_name = "unknown";
        
        switch (e->log_type) {
            case 's': case 'f': case 'r': case 'e':
            case 'l': case 'k': case 'i': case 'o':

                if (e->task != NULL) {
                    task_name = pcTaskGetName((TaskHandle_t)e->task);
                }
                
                printf("[%3"PRIu32"] tick=%6"PRIu32" time=%10"PRIu64"us queue=%p wait=%3"PRIu32" task=%s type=%c \n",
                    i,
                    (uint32_t)e->tick,
                    (uint64_t)e->timestamp_us,
                    (void*)e->queue,
                    (uint32_t)e->block_time,
                    task_name,
                    e->log_type);

                break;

            case 't':
                printf("[%3"PRIu32"] tick=%6"PRIu32" time=%10"PRIu64"us task=%s new_tick=%6"PRIu32" type=%c \n",
                    i,
                    (uint32_t)e->tick,
                    (uint64_t)e->timestamp_us,
                    (const char*)e->identifier,
                    (uint32_t)e->new_tick,
                    e->log_type);
                break;

            default:
                break;
        }
    }

    printf("==========================================\n\n");
}
