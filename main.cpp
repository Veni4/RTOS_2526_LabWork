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
QueueHandle_t PrinterQueue;
GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> display(WatchyDisplay{});

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
        xQueueSend(PrinterQueue, &str, 0);
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
        ESP_LOGI("vPrinter", "%s", xMessage);
        vTaskDelayUntil(&xLastWakeTime, TICKS_PER_MS*0.1);
    } 
}

#define traceQUEUE_SEND()
 log_event(xLastWakeTime, str);

#define traceQUEUE_RECIEVE()
 log_event(xLastWakeTime, xMessage);

#define INCLUDE_vTaskDelete 1

extern "C" void app_main() {
    /* Only priorities from 1-25 (configMAX_PRIORITIES) possible. */
    /* Initialize the display first. */
    
    //xTaskCreate(initDisplay, "initDisplay", 4096, NULL, configMAX_PRIORITIES-1, NULL);
    //xTaskCreate(buttonWatch, "watch", 8192, NULL, 1, NULL);
    //xTaskCreate(vPeriodicCounter, "counter", 8192, NULL, configMAX_PRIORITIES-2, NULL);
    xTaskCreate(vPrinter, "reciever", 4096, NULL, 4, NULL);
    xTaskCreate(vPrint1, "sender 1", 4096, NULL, 3, NULL);
    xTaskCreate(vPrint2, "sender 2", 4096, NULL, 3, NULL);
    xTaskCreate(vPrint3, "sender 3", 4096, NULL, 3, NULL);

    ESP_LOGI("app_main", "Starting scheduler from app_main()");
    vTaskStartScheduler();
    /* vTaskStartScheduler is blocking - this should never be reached */
    ESP_LOGE("app_main", "insufficient RAM! aborting");
    abort();
} 
