/**
 * @file trace_log.c
 * @brief Implementation of FreeRTOS trace logging system
 * 
 * This module implements a thread-safe circular buffer for capturing FreeRTOS
 * events with microsecond precision. Uses critical sections for atomic updates.
 */

#include "trace_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <inttypes.h>

uint32_t logging_while_printing = 0;
uint32_t enable_logging = 1;

// Just to count how many logs would be missed if we don't allow new logs while printing
volatile uint32_t dropped_logs = 0;

struct trace_log_entry trace_log_buffer[TRACE_LOG_BUFFER_SIZE];
volatile uint32_t trace_log_head = 0;

/* Static mutex for thread-safe access to trace log buffer */
static portMUX_TYPE trace_log_mux = portMUX_INITIALIZER_UNLOCKED;


/**
 * @brief Convert trace log type to string token
 * @param t Trace log type enum
 * @return Two-character string token for log type
 */
static const char *trace_token(trace_log_type_t t)
{
    switch (t) {
        case TRACE_LOG_QUEUE_SEND:                   return "traceQUEUE_SEND";
        case TRACE_LOG_QUEUE_SEND_FAILED:            return "traceQUEUE_SEND_FAILED";
        case TRACE_LOG_QUEUE_SEND_FROM_ISR:          return "traceQUEUE_SEND_FROM_ISR";
        case TRACE_LOG_QUEUE_SEND_FROM_ISR_FAILED:   return "traceQUEUE_SEND_FROM_ISR_FAILED";
        case TRACE_LOG_QUEUE_RECEIVE:                return "traceQUEUE_RECEIVE";
        case TRACE_LOG_QUEUE_RECEIVE_FAILED:         return "traceQUEUE_RECEIVE_FAILED";
        case TRACE_LOG_QUEUE_RECEIVE_FROM_ISR:       return "traceQUEUE_RECEIVE_FROM_ISR";
        case TRACE_LOG_QUEUE_RECEIVE_FROM_ISR_FAILED:return "traceQUEUE_RECEIVE_FROM_ISR_FAILED";

        case TRACE_LOG_TASK_INCREMENT_TICK:          return "traceTASK_INCREMENT_TICK";

        case TRACE_LOG_TASK_CREATE:                  return "traceTASK_CREATE";
        case TRACE_LOG_TASK_CREATE_FAILED:           return "traceTASK_CREATE_FAILED";
        case TRACE_LOG_TASK_DELETE:                  return "traceTASK_DELETE";

        case TRACE_LOG_TASK_DELAY:                   return "traceTASK_DELAY";
        case TRACE_LOG_TASK_DELAY_UNTIL:             return "traceTASK_DELAY_UNTIL";

        case TRACE_LOG_TASK_SWITCHED_IN:             return "traceTASK_SWITCHED_IN";
        case TRACE_LOG_TASK_SWITCHED_OUT:            return "traceTASK_SWITCHED_OUT";

        default:                                     return "??";
    }
}


void add_trace_log_entry(unsigned long tick, unsigned long long timestamp_us, void* queue, unsigned long block_time, void* task, trace_log_type_t log_type)
{
    if(enable_logging == 1) {
        struct trace_log_entry e = {
            .tick = (TickType_t)tick,
            .timestamp_us = (uint64_t)timestamp_us,
            .queue = (QueueHandle_t)queue,
            .block_time = (TickType_t)block_time,
            .task = (TaskHandle_t)task,
            .log_type = log_type
        };
        /* Thread-safe: use mutex to atomically update head */
        taskENTER_CRITICAL(&trace_log_mux);
        uint32_t i = trace_log_head % TRACE_LOG_BUFFER_SIZE;
        trace_log_buffer[i] = e;
        trace_log_head++;
        taskEXIT_CRITICAL(&trace_log_mux);
    } else {
        taskENTER_CRITICAL(&trace_log_mux);
        dropped_logs = dropped_logs + 1; 
        taskEXIT_CRITICAL(&trace_log_mux);
    }
    
}

void add_trace_log_entry_newtick(unsigned long old_tick, unsigned long long timestamp_us, const char* identifier, unsigned long new_tick, void* task, trace_log_type_t log_type)
{
    if(enable_logging == 1) {
        struct trace_log_entry e = {
            .tick = (TickType_t)old_tick,
            .timestamp_us = (uint64_t)timestamp_us,
            .identifier = (const char*)identifier,
            .new_tick = (TickType_t)new_tick,
            .task = (TaskHandle_t)task,
            .log_type = log_type
        };
        /* Thread-safe: use mutex to atomically update head */
        taskENTER_CRITICAL(&trace_log_mux);
        uint32_t i = trace_log_head % TRACE_LOG_BUFFER_SIZE;
        trace_log_buffer[i] = e;
        trace_log_head++;
        taskEXIT_CRITICAL(&trace_log_mux);
    } else {
        taskENTER_CRITICAL(&trace_log_mux);
        dropped_logs = dropped_logs + 1; 
        taskEXIT_CRITICAL(&trace_log_mux);
    }
}

void add_trace_log_entry_identifier(unsigned long tick, unsigned long long timestamp_us, const char* identifier, void* task, trace_log_type_t log_type)
{
    if(enable_logging == 1) {
        struct trace_log_entry e = {
            .tick = (TickType_t)tick,
            .timestamp_us = (uint64_t)timestamp_us,
            .identifier = (const char*)identifier,
            .task = (TaskHandle_t)task,
            .log_type = log_type
        };
        /* Thread-safe: use mutex to atomically update head */
        taskENTER_CRITICAL(&trace_log_mux);
        uint32_t i = trace_log_head % TRACE_LOG_BUFFER_SIZE;
        trace_log_buffer[i] = e;
        trace_log_head++;
        taskEXIT_CRITICAL(&trace_log_mux);
    } else {
        taskENTER_CRITICAL(&trace_log_mux);
        dropped_logs = dropped_logs + 1; 
        taskEXIT_CRITICAL(&trace_log_mux);
    }
}

void add_trace_log_entry_event(unsigned long tick, unsigned long long timestamp_us, void* task, const char* event, trace_log_type_t log_type)
{
    if(enable_logging == 1) {
        struct trace_log_entry e = {
            .tick = (TickType_t)tick,
            .timestamp_us = (uint64_t)timestamp_us,
            .identifier = (const char*)event,
            .task = (TaskHandle_t)task,
            .log_type = log_type
        };
        /* Thread-safe: use mutex to atomically update head */
        taskENTER_CRITICAL(&trace_log_mux);
        uint32_t i = trace_log_head % TRACE_LOG_BUFFER_SIZE;
        trace_log_buffer[i] = e;
        trace_log_head++;
        taskEXIT_CRITICAL(&trace_log_mux);
    } else {
        taskENTER_CRITICAL(&trace_log_mux);
        dropped_logs = dropped_logs + 1; 
        taskEXIT_CRITICAL(&trace_log_mux);
    }
}

void print_trace_logs(void)
{
    taskENTER_CRITICAL(&trace_log_mux);
    if(logging_while_printing == 0) {
        enable_logging = 0;
    }
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
        // in any case it will print 
        // <token> <tick> <us> <task_handle> <queue_handle> <wait_tick> <newtick> <taskname>
        // the unused fields will be filled with "-" to make parsing easier on python end
        switch (e->log_type) {
            // Queue-related logs
            case TRACE_LOG_QUEUE_SEND:
            case TRACE_LOG_QUEUE_SEND_FAILED:
            case TRACE_LOG_QUEUE_RECEIVE:
            case TRACE_LOG_QUEUE_RECEIVE_FAILED:
            case TRACE_LOG_QUEUE_RECEIVE_FROM_ISR:
            case TRACE_LOG_QUEUE_RECEIVE_FROM_ISR_FAILED:

                if (e->task != NULL) {
                    task_name = pcTaskGetName((TaskHandle_t)e->task);
                }
                // <token> <tick> <us> <task_handle> <queue_handle> <wait_tick> <newtick> <taskname>
                printf("%s %"PRIu32" %"PRIu64" %p %p %"PRIu32" - %s\n",
                    trace_token(e->log_type),
                    (uint32_t)e->tick,
                    (uint64_t)e->timestamp_us,
                    (void*)e->task,
                    (void*)e->queue,
                    (uint32_t)e->block_time,
                    task_name);

                break;

            // Queue send from ISR logs (also uses queue format)
            case TRACE_LOG_QUEUE_SEND_FROM_ISR:
            case TRACE_LOG_QUEUE_SEND_FROM_ISR_FAILED:

                if (e->task != NULL) {
                    task_name = pcTaskGetName((TaskHandle_t)e->task);
                }
                //<token> <tick> <us> <task_handle> <queue_handle> <wait_tick> <newtick> <taskname>                
                printf("%s %"PRIu32" %"PRIu64" %p %p %"PRIu32" %s\n",
                    trace_token(e->log_type),
                    (uint32_t)e->tick,
                    (uint64_t)e->timestamp_us,
                    (void*)e->task,
                    (void*)e->queue,
                    (uint32_t)e->block_time,
                    task_name);

                break;

            // Tick increment logs
            case TRACE_LOG_TASK_INCREMENT_TICK:
                // <token> <tick> <us> <task_handle> <queue_handle> <wait_tick> <newtick> <taskname>
                printf("%s %"PRIu32" %"PRIu64" - - - %"PRIu32" - \n",
                    trace_token(e->log_type),
                    (uint32_t)e->tick,
                    (uint64_t)e->timestamp_us,
                    (uint32_t)e->new_tick);
                break; 

            // Task creation and deletion logs
            case TRACE_LOG_TASK_CREATE:
            case TRACE_LOG_TASK_CREATE_FAILED:
            case TRACE_LOG_TASK_DELETE:

                if (e->task != NULL) {
                    task_name = pcTaskGetName((TaskHandle_t)e->task);
                }
                // <token> <tick> <us> <task_handle> <queue_handle> <wait_tick> <newtick> <taskname>
                printf("%s %"PRIu32" %"PRIu64" %p - - - %s \n",
                    trace_token(e->log_type),
                    (uint32_t)e->tick,
                    (uint64_t)e->timestamp_us,
                    (void*)e->task,
                    task_name);
                break;

            // Task delay logs
            case TRACE_LOG_TASK_DELAY:
            case TRACE_LOG_TASK_DELAY_UNTIL:
                if (e->task != NULL) {
                    task_name = pcTaskGetName((TaskHandle_t)e->task);
                }

                //if(e->new_tick == 0) what do if simple delay?
                // <token> <tick> <us> <task_handle> <queue_handle> <wait_tick> <newtick> <taskname>
                printf("%s %"PRIu32" %"PRIu64" %p - - %"PRIu32" %s \n",
                    trace_token(e->log_type),
                    (uint32_t)e->tick,
                    (uint64_t)e->timestamp_us,
                    (void*)e->task,
                    (uint32_t)e->new_tick,
                    task_name);
                break;

            // Task switch logs
            case TRACE_LOG_TASK_SWITCHED_IN:
            case TRACE_LOG_TASK_SWITCHED_OUT:
                if (e->task != NULL) {
                    task_name = pcTaskGetName((TaskHandle_t)e->task);
                }
                // <token> <tick> <us> <task_handle> <queue_handle> <wait_tick> <newtick> <taskname>
                printf("%s %"PRIu32" %"PRIu64" %p - - - %s \n",
                    trace_token(e->log_type),
                    (uint32_t)e->tick,
                    (uint64_t)e->timestamp_us,
                    (void*)e->task,
                    task_name);

                break;

            default:
                break;
        }
    }

    printf("==========================================\n\n");
    printf("dropped_logs %"PRIu32" - - - - - - \n", (uint32_t)dropped_logs);

    taskENTER_CRITICAL(&trace_log_mux);
    dropped_logs = 0;
    enable_logging = 1;
    taskEXIT_CRITICAL(&trace_log_mux);


}
