#pragma once

/**
 * @file trace_log.h
 * @brief FreeRTOS trace logging system for debugging and performance analysis
 * 
 * This module provides a circular buffer-based logging system to capture
 * FreeRTOS events (task switches, queue operations, delays, etc.) with
 * microsecond timestamps for later analysis.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdint.h>



/**
 * @brief Trace log entry structure
 * 
 * Captures a single trace event with timing information and context.
 * Note: Enum trace_log_type_t is defined in FreeRTOSConfig.h (included via FreeRTOS.h)
 */
struct trace_log_entry {
    TickType_t       tick;          /**< FreeRTOS tick count*/
    uint64_t         timestamp_us;  /**< Microsecond timestamp from ESP */
    QueueHandle_t    queue;         /**< Queue handle (for queue operations) */
    TickType_t       block_time;    /**< Requested blocking time */
    TaskHandle_t     task;          /**< Task handle associated with event */
    const char*      identifier;    /**< String identifier (task name, event) */
    TickType_t       new_tick;      /**< New tick value (for tick changes)*/
    trace_log_type_t log_type;      /**< Type of trace event */
};

#define TRACE_LOG_BUFFER_SIZE 1024
extern struct trace_log_entry trace_log_buffer[TRACE_LOG_BUFFER_SIZE];
extern volatile uint32_t trace_log_head;

/**
 * @brief Add a standard trace log entry
 */
void add_trace_log_entry(unsigned long tick, unsigned long long timestamp_us, void* queue, unsigned long block_time, void* task, trace_log_type_t log_type);

/**
 * @brief Add trace entry with tick change information
 */
void add_trace_log_entry_newtick(unsigned long old_tick, unsigned long long timestamp_us, const char* identifier, unsigned long new_tick, void* task, trace_log_type_t log_type);

/**
 * @brief Add trace entry with string identifier
 */
void add_trace_log_entry_identifier(unsigned long tick, unsigned long long timestamp_us, const char* identifier, void* task, trace_log_type_t log_type);

/**
 * @brief Add trace entry for generic event
 */
void add_trace_log_entry_event(unsigned long tick, unsigned long long timestamp_us, void* task, const char* event, trace_log_type_t log_type);

/**
 * @brief Print all trace logs to console
 * 
 * Outputs the circular buffer contents in human-readable format
 */
void print_trace_logs(void);


void flush_logs_to_file(void);


#ifdef __cplusplus
}
#endif