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
GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> display(WatchyDisplay{});
char* log_buffer[256] = {0};
int log_index = 0;
char str_buffer[64] = {0};  // Changed to array instead of pointer
TaskHandle_t sender1 = NULL;
TaskHandle_t sender2 = NULL;
TaskHandle_t sender3 = NULL;
TaskHandle_t reciever = NULL;

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


// Function to create log entries
char* log_event(TickType_t timestamp, const char* message) {
    char* log_entry = (char*)pvPortMalloc(64 * sizeof(char));
    if (log_entry != NULL) {
        snprintf(log_entry, 64, "Time: %u, Msg: %s", (unsigned int)timestamp, message);
    }
    return log_entry;
}


void initDisplay(void* pvParameters) {
    ESP_LOGI("initDisplay", "initializing display");

    /* Setting gpio pin types, always necessary at the start. */
    pinMode(DISPLAY_CS, OUTPUT);
    pinMode(DISPLAY_RES, OUTPUT);
    pinMode(DISPLAY_DC, OUTPUT);
    pinMode(DISPLAY_BUSY, OUTPUT);
    pinMode(BOTTOM_LEFT, INPUT);
    pinMode(BOTTOM_RIGHT, INPUT);
    pinMode(TOP_LEFT, INPUT);
    pinMode(TOP_RIGHT, INPUT);

    /* Init the display. */
    display.epd2.initWatchy();
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold24pt7b);
    display.setCursor(0, 90);
    display.print("Ilmari\nMiguel\nasdf\n");
    display.display(false);

    /* Delete the display initialization task. */
    ESP_LOGI("initDisplay", "finished display initialization");
    vTaskDelete(NULL);
}

void buttonWatch(void* pvParameters) {
    unsigned int refresh = 0;

    for (;;) {
        if (digitalRead(BOTTOM_LEFT) == HIGH) {
            ESP_LOGI("buttonWatch", "Bottom Left pressed!");
            display.fillRoundRect(0, 150, 50, 50, 20, GxEPD_BLACK);
            display.display(true);
            vTaskDelay(500);
            display.fillRoundRect(0, 150, 50, 50, 20, GxEPD_WHITE);
            display.display(true);
            refresh++;
        } else if (digitalRead(BOTTOM_RIGHT) == HIGH) {
            ESP_LOGI("buttonWatch", "Bottom Right pressed!");
            display.fillRoundRect(150, 150, 50, 50, 20, GxEPD_BLACK);
            display.display(true);
            vTaskDelay(500);
            display.fillRoundRect(150, 150, 50, 50, 20, GxEPD_WHITE);
            display.display(true);
            refresh++;
        } else if (digitalRead(TOP_LEFT) == HIGH) {
            ESP_LOGI("buttonWatch", "Top Left pressed!");
            display.fillRoundRect(0, 0, 50, 50, 20, GxEPD_BLACK);
            display.display(true);
            vTaskDelay(500);
            display.fillRoundRect(0, 0, 50, 50, 20, GxEPD_WHITE);
            display.display(true);
            refresh++;
        } else if (digitalRead(TOP_RIGHT) == HIGH) {
            ESP_LOGI("buttonWatch", "Top Right pressed!");
            display.fillRoundRect(150, 0, 50, 50, 20, GxEPD_BLACK);
            display.display(true);
            vTaskDelay(500);
            display.fillRoundRect(150, 0, 50, 50, 20, GxEPD_WHITE);
            display.display(true);
            refresh++;
        } else if (refresh >= 10) {
            ESP_LOGI("buttonWatch", "Performing full refresh of display");
            display.display(false);
            refresh = 0;
        }
    }
}

void vPeriodicCounter(void* pvParameters) {
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    unsigned int count = 0;
    display.fillScreen(GxEPD_WHITE);
    display.display(false);

    for (;;) {
        display.fillScreen(GxEPD_WHITE);

        display.display(false);
        display.setCursor(0, 35);
        display.printf("%u",count);
        display.print("\n");

        display.display(true);
        count = count + 1;
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_MS*5);
    }
}


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
        traceQUEUE_SEND(PrinterQueue);
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
        traceQUEUE_SEND(PrinterQueue);
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
        traceQUEUE_SEND(PrinterQueue);
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
        traceQUEUE_RECIEVE(PrinterQueue);
        //log_buffer[log_index++] = traceQUEUE_RECIEVE();
        //  ESP_LOGI("vPrinter", "%s", xMessage);
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_MS*0.1);
    } 
}

void vLogger (void* pvParameters){

    //deletes all previous tasks
    if (sender1 != NULL){
        vTaskDelete(sender1);
    }
    if (sender2 != NULL){
        vTaskDelete(sender2);
    }
    if (sender3 != NULL){
        vTaskDelete(sender3);
    }
    if (reciever != NULL){
        vTaskDelete(reciever);
    }

    //prints all previous logs
    ESP_LOGI("vLogger", "Logging events:");
    ESP_LOGI("vLogger", "Number of logs: %d", log_index);
    for (int i = 0; i < log_index - 1; i++){
        if (log_buffer[i] != NULL) {
            ESP_LOGI("vLogger", "%s", log_buffer[i]);
        } else {
            ESP_LOGI("vLogger", "Entry %d: NULL", i);
        }
    }
    
    // Delete this task when done
    vTaskDelete(NULL);
}

extern "C" void app_main() {
    /* Only priorities from 1-25 (configMAX_PRIORITIES) possible. */
    /* Initialize the display first. */
    
    //xTaskCreate(initDisplay, "initDisplay", 4096, NULL, configMAX_PRIORITIES-1, NULL);
    //xTaskCreate(buttonWatch, "watch", 8192, NULL, 1, NULL);
    //xTaskCreate(vPeriodicCounter, "counter", 8192, NULL, configMAX_PRIORITIES-2, NULL);
    xTaskCreate(vPrinter, "reciever", 4096, NULL, 4, &reciever);
    xTaskCreate(vPrint1, "sender 1", 4096, NULL, 3, &sender1);
    xTaskCreate(vPrint2, "sender 2", 4096, NULL, 3, &sender2);
    xTaskCreate(vPrint3, "sender 3", 4096, NULL, 3, &sender3);
    xTaskCreate(vLogger, "logger", 8192, NULL, 2, NULL);

    ESP_LOGI("app_main", "Starting scheduler from app_main()");
    vTaskStartScheduler();
    /* vTaskStartScheduler is blocking - this should never be reached */
    ESP_LOGE("app_main", "insufficient RAM! aborting");
    abort();
} 
