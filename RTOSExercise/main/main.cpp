#include <Arduino.h>
#include <Display.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxEPD2_BW.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <Watchy.h>

#define TICKS_PER_MS 1000

#define BOTTOM_LEFT 26
#define TOP_LEFT 25 
#define BOTTOM_RIGHT 4
#define TOP_RIGHT 35
#define DISPLAY_CS 5
#define DISPLAY_RES 9
#define DISPLAY_DC 10
#define DISPLAY_BUSY 19

//Global variables, can be accessed by all tasks
//Could be optimized by passing these as parameters to tasks instead
QueueHandle_t PrinterQueue;

/* Global counter used by FreeRTOS trace macro traceQUEUE_SEND().
 *
 * Defined with C linkage so that the C FreeRTOS kernel code can
 * reference it (the declaration is in FreeRTOSConfig.h).
 */
extern "C" volatile uint32_t g_queue_send_count = 0;
GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> display(WatchyDisplay{});
char* log_buffer[256] = {0};
int log_index = 0;
char str_buffer[64] = {0};  // Changed to array instead of pointer
TaskHandle_t sender1 = NULL;
TaskHandle_t sender2 = NULL;
TaskHandle_t sender3 = NULL;
TaskHandle_t reciever = NULL;
/*
#define traceQUEUE_SEND(pxQueue) \
    do { \
        TickType_t xLastWakeTime = xTaskGetTickCount(); \
        uint32_t uxLastWakeTimeSecs = xLastWakeTime / TICKS_PER_MS; \
        if (log_index < 255) { \
            char* log_entry = new_log_event(xLastWakeTime, uxLastWakeTimeSecs, pxQueue, 0, xTaskGetCurrentTaskHandle()); \
            if (log_entry != NULL) { \
                log_buffer[log_index] = log_entry; \
                ESP_LOGI("traceQUEUE_SEND", "Entry %d: %s", log_index, log_buffer[log_index]); \
                log_index++; \
            } \
        } \
    } while(0)

#define traceQUEUE_RECIEVE(pxQueue) \
    do { \
        TickType_t xLastWakeTime = xTaskGetTickCount(); \
        uint32_t uxLastWakeTimeSecs = xLastWakeTime / TICKS_PER_MS; \
        if (log_index < 255) { \
            char* log_entry = new_log_event(xLastWakeTime, uxLastWakeTimeSecs, pxQueue, 0, xTaskGetCurrentTaskHandle()); \
            if (log_entry != NULL) { \
                log_buffer[log_index] = log_entry; \
                ESP_LOGI("traceQUEUE_RECIEVE", "Entry %d: %s", log_index, log_buffer[log_index]); \
                log_index++; \
            } \
        } \
    } while(0)

char* new_log_event(TickType_t tick_count, uint32_t time_seconds, QueueHandle_t queue, TickType_t wait_tick_time, TaskHandle_t task_handle){
    char* log_entry = (char*)pvPortMalloc(128 * sizeof(char));
    if (log_entry != NULL) {
        const char* task_name = pcTaskGetName(task_handle);
        // Calculate milliseconds for more precision
        uint32_t time_ms = tick_count * portTICK_PERIOD_MS;
        uint32_t seconds = time_ms / 1000;
        uint32_t milliseconds = time_ms % 1000;
        
        snprintf(log_entry, 128, "Time: %lu ticks, %lu.%03lus, Task: %s, Queue: %p, WaitTime: %u", 
                 tick_count, seconds, milliseconds, task_name, queue, (unsigned int)wait_tick_time);
    }
    return log_entry;
}
*/
// Define custom trace macros that work with your logging system
/* #define traceQUEUE_SEND(pxQueue) \
    do { \
        TickType_t xLastWakeTime = xTaskGetTickCount(); \
        if (log_index < 255) { \
            char* log_entry = log_event(xLastWakeTime, str_buffer); \
            if (log_entry != NULL) { \
                log_buffer[log_index] = log_entry; \
                ESP_LOGI("traceQUEUE_SEND", "Entry %d: %s", log_index, log_buffer[log_index]); \
                log_index++; \
            } \
        } \
    } while(0) */

/*
#define traceQUEUE_RECIEVE(pxQueue) \ 
    do { \
        TickType_t xLastWakeTime = xTaskGetTickCount(); \
        if (log_index < 255) { \
            char* log_entry = log_event(xLastWakeTime, str_buffer); \
            if (log_entry != NULL) { \
                log_buffer[log_index] = log_entry; \
                ESP_LOGI("traceQUEUE_RECIEVE", "Entry %d: %s", log_index, log_buffer[log_index]); \
                log_index++; \
            } \
        } \
    } while(0)
*/

#define INCLUDE_vTaskDelete 1

/*
// Function to create log entries
char* log_event(TickType_t timestamp, const char* message) {
    char* log_entry = (char*)pvPortMalloc(64 * sizeof(char));
    if (log_entry != NULL) {
        snprintf(log_entry, 64, "Time: %u, Msg: %s", (unsigned int)timestamp, message);
    }
    return log_entry;
}
*/


void vPrint1(void* pvParameters){
    ESP_LOGI("vPrint1", "Initializing printer1");
    TickType_t xLastWakeTime;
    char* str;
    //char counter = 0;
    str = (char*) pvPortMalloc(25*sizeof(char));

    xLastWakeTime = xTaskGetTickCount();
    ESP_LOGI("vPrinter", "Printer1 initialized");

    for (;;) {
        sprintf(str, "Message: 1");
        sprintf(str_buffer, "Message: 1");
        xQueueSend(PrinterQueue, &str, 0);
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_MS*0.1);
    }
}
void vPrint2(void* pvParameters){
    TickType_t xLastWakeTime;
    char* str;
    //char counter = 0;
    str = (char*) pvPortMalloc(25*sizeof(char));

    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        sprintf(str, "Message: 2");
        sprintf(str_buffer, "Message: 2");
        xQueueSend(PrinterQueue, &str, 0);
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_MS*0.2);
    }
}
void vPrint3(void* pvParameters){
    TickType_t xLastWakeTime;
    char* str;
    //char counter = 0; 
    str = (char*) pvPortMalloc(25*sizeof(char));

    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        sprintf(str, "Message: 3");
        sprintf(str_buffer, "Message: 3");
        xQueueSend(PrinterQueue, &str, 0);
        //ESP_LOGI("vPrinter", "%s", str);
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_MS*0.3);
    }
}
void vPrinter(void* pvParameters){
    ESP_LOGI("vPrinter", "Initializing reciever");
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    char * xMessage;
    PrinterQueue = xQueueCreate(10, sizeof( char* ));
    
    ESP_LOGI("vPrinter", "Reciever initialized");
    for (;;) {
        xQueueReceive( PrinterQueue, &xMessage, portMAX_DELAY);
        // Periodically report how many times queues have been sent to
        ESP_LOGI("vPrinter", "Queue send count: %u", (unsigned)g_queue_send_count);
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_MS*0.1);
    } 
}


extern "C" void app_main() {

    xTaskCreate(vPrinter, "reciever", 4096, NULL, 4, &reciever);
    xTaskCreate(vPrint1, "sender 1", 4096, NULL, 3, &sender1);
    xTaskCreate(vPrint2, "sender 2", 4096, NULL, 3, &sender2);
    xTaskCreate(vPrint3, "sender 3", 4096, NULL, 3, &sender3);

    ESP_LOGI("app_main", "Starting scheduler from app_main()");
    vTaskStartScheduler();
    /* vTaskStartScheduler is blocking - this should never be reached */
    ESP_LOGE("app_main", "insufficient RAM! aborting");
    abort();
} 
