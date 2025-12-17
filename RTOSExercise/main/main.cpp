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

#define TICKS_PER_S 1000

#define BOTTOM_LEFT 26
#define TOP_LEFT 25 
#define BOTTOM_RIGHT 4
#define TOP_RIGHT 35
#define DISPLAY_CS 5
#define DISPLAY_RES 9
#define DISPLAY_DC 10
#define DISPLAY_BUSY 19

QueueHandle_t PrinterQueue;

GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> display(WatchyDisplay{});
char* log_buffer[256] = {0};
int log_index = 0;
char str_buffer[64] = {0};
TaskHandle_t sender1 = NULL;
TaskHandle_t sender2 = NULL;
TaskHandle_t sender3 = NULL;
TaskHandle_t reciever = NULL;


void vPrint1(void* pvParameters){
    ESP_LOGI("vPrint1", "Initializing printer1");
    TickType_t xLastWakeTime;
    char* str;
    str = (char*) pvPortMalloc(25*sizeof(char));

    xLastWakeTime = xTaskGetTickCount();
    ESP_LOGI("vPrinter", "Printer1 initialized");

    for (;;) {
        sprintf(str, "Message: 1");
        sprintf(str_buffer, "Message: 1");
        xQueueSend(PrinterQueue, &str, 0);
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_S*0.1);
    }
}
void vPrint2(void* pvParameters){
    TickType_t xLastWakeTime;
    char* str;
    str = (char*) pvPortMalloc(25*sizeof(char));

    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        sprintf(str, "Message: 2");
        sprintf(str_buffer, "Message: 2");
        xQueueSend(PrinterQueue, &str, 0);
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_S*0.2);
    }
}
void vPrint3(void* pvParameters){
    TickType_t xLastWakeTime;
    char* str;
    str = (char*) pvPortMalloc(25*sizeof(char));

    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        sprintf(str, "Message: 3");
        sprintf(str_buffer, "Message: 3");
        xQueueSend(PrinterQueue, &str, 0);
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_S*0.3);
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
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_S*0.1);
    } 
}


/* Task to periodically print trace logs */
void vTraceLogger(void* pvParameters) {
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    ESP_LOGI("vTraceLogger", "Trace logger task started");
    unsigned long log_count = 0;
    for (;;) {
        ESP_LOGI("vPrinter", "Log count: %u", (unsigned)log_count);
        // The printing takes so long that it can't really printed more often
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_S * 7); //print every 7 seconds
        print_trace_logs();
        log_count = log_count + 1;
    }
}

extern "C" void app_main() {

    xTaskCreate(vPrinter, "reciever", 4096, NULL, 4, &reciever);
    xTaskCreate(vPrint1, "sender-1", 4096, NULL, 3, &sender1);
    xTaskCreate(vPrint2, "sender-2", 4096, NULL, 3, &sender2);
    xTaskCreate(vPrint3, "sender-3", 4096, NULL, 3, &sender3);
    
    xTaskCreate(vTraceLogger, "trace_lg", 4096, NULL, 2, NULL);

    ESP_LOGI("app_main", "Starting scheduler from app_main()");
    vTaskStartScheduler();
    /* vTaskStartScheduler is blocking - this should never be reached */
    ESP_LOGE("app_main", "insufficient RAM! aborting");
    abort();
} 
