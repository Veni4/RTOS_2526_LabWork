#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include "dt.hpp"
#define TICKS_PER_MS 1000

DecisionTreeWorkload wl;

void periodic_task(void* arg) {
    TickType_t lastWakeTme = xTaskGetTickCount();
    const TickType_t period = 1000;

    for (;;) {
        xTaskDelayUntil(&lastWakeTme, period);
        TickType_t before = xTaskGetTickCount();

        double accuracy = wl.executeTree();

        TickType_t after = xTaskGetTickCount();
        ESP_LOGI("periodic_task", "Accuracy: %f", accuracy);

        ESP_LOGI("periodic_task", "Took %ld ticks", after - before);
        if (xTaskGetTickCount() > lastWakeTme + period) {
            ESP_LOGE("periodic_task", "Task missed the deadline");
        }
    }
}

extern "C" void app_main() {
    xTaskCreate(periodic_task, "periodic", 4096, NULL, 1, NULL);

    ESP_LOGI("app_main", "Starting scheduler from app_main()");
    vTaskStartScheduler();
    /* vTaskStartScheduler is blocking - this should never be reached */
    ESP_LOGE("app_main", "insufficient RAM! aborting");
    abort();
}