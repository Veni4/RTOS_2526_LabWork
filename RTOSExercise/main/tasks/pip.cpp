#include <Arduino.h>
#include <Display.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxEPD2_BW.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <Watchy.h>
#include "trace_log.h"
#include "pip.h"

#define USE_PIP 0

#define TICKS_PER_S 1000

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

GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> display(WatchyDisplay{});
char* log_buffer[256] = {0};
int log_index = 0;
char str_buffer[64] = {0};  // Changed to array instead of pointer
TaskHandle_t sender1 = NULL;
TaskHandle_t sender2 = NULL;
TaskHandle_t sender3 = NULL;
TaskHandle_t reciever = NULL;

static const char *TAG = "pip_demo";

#if USE_PIP
static pip_mutex_t *g_mutex = NULL;
#else
static SemaphoreHandle_t g_mutex = NULL;
#endif

void low_task(void* pv) {
    ESP_LOGI(TAG, "Low starting, priority=%u", (unsigned)uxTaskPriorityGet(NULL));
#if USE_PIP
    pip_mutex_lock(g_mutex, portMAX_DELAY);
#else
    xSemaphoreTake(g_mutex, portMAX_DELAY);
#endif
    ESP_LOGI(TAG, "Low acquired mutex, priority=%u", (unsigned)uxTaskPriorityGet(NULL));

    // Simulate long critical section so Medium can preempt if PIP not enabled
    for (int i = 0; i < 10; ++i) {
        ESP_LOGI(TAG, "Low working %d (prio %u)", i, (unsigned)uxTaskPriorityGet(NULL));
        vTaskDelay(pdMS_TO_TICKS(200));
    }
if USE_PIP
    pip_mutex_unlock(g_mutex);
#else
    xSemaphoreGive(g_mutex);
#endif
    ESP_LOGI(TAG, "Low released mutex and done");
    vTaskDelete(NULL);
}

void medium_task(void* pv) {
    ESP_LOGI(TAG, "Medium starting, priority=%u", (unsigned)uxTaskPriorityGet(NULL));
    // Medium is CPU-bound-ish to preempt Low if Low has lower priority
    for (int i = 0; i < 25; ++i) {
        ESP_LOGI(TAG, "Medium tick %d (prio %u)", i, (unsigned)uxTaskPriorityGet(NULL));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "Medium done");
    vTaskDelete(NULL);
}

void high_task(void* pv) {
    ESP_LOGI(TAG, "High starting, will try to acquire after delay");
    vTaskDelay(pdMS_TO_TICKS(700)); // ensure Low already holds mutex, Medium active
    ESP_LOGI(TAG, "High trying to take mutex, priority=%u", (unsigned)uxTaskPriorityGet(NULL));
#if USE_PIP
    pip_mutex_lock(g_mutex, portMAX_DELAY);
#else
    xSemaphoreTake(g_mutex, portMAX_DELAY);
#endif
    ESP_LOGI(TAG, "High acquired mutex (should be after Low finishes). priority=%u", (unsigned)uxTaskPriorityGet(NULL));
#if USE_PIP
    pip_mutex_unlock(g_mutex);
#else
    xSemaphoreGive(g_mutex);
#endif
    ESP_LOGI(TAG, "High released mutex and done");
    vTaskDelete(NULL);
}


/* Task to periodically print trace logs */
void vTraceLogger(void* pvParameters) {
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    ESP_LOGI("vTraceLogger", "Trace logger task started");
    unsigned long log_count = 0;
    for (;;) {
        ESP_LOGI("vPrinter", "Log count: %u", (unsigned)log_count);

        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_S * 2); //print every 2 seconds
        print_trace_logs();
        log_count = log_count + 1;
    }
}

extern "C" void app_main() {

    xTaskCreate(vPrinter, "reciever", 4096, NULL, 4, &reciever);
    xTaskCreate(vPrint1, "sender 1", 4096, NULL, 3, &sender1);
    xTaskCreate(vPrint2, "sender 2", 4096, NULL, 3, &sender2);
    xTaskCreate(vPrint3, "sender 3", 4096, NULL, 3, &sender3);
    
    /* Create trace logger task with lower priority */
    xTaskCreate(vTraceLogger, "trace_lg", 4096, NULL, 2, NULL);

    ESP_LOGI("app_main", "Starting scheduler from app_main()");
    vTaskStartScheduler();
    /* vTaskStartScheduler is blocking - this should never be reached */
    ESP_LOGE("app_main", "insufficient RAM! aborting");
    abort();
} 
