#include "pip.h"
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "PIP";

pip_mutex_t * pip_mutex_create(void)
{
    pip_mutex_t *m = (pip_mutex_t*)pvPortMalloc(sizeof(pip_mutex_t));
    if (!m) return NULL;
    m->sem = xSemaphoreCreateBinary();
    if (m->sem == NULL) {
        vPortFree(m);
        return NULL;
    }
    // start unlocked
    xSemaphoreGive(m->sem);
    m->owner = NULL;
    m->owner_orig_prio = 0;
    m->lock_count = 0;
    return m;
}

BaseType_t pip_mutex_lock(pip_mutex_t *m, TickType_t ticks_to_wait)
{
    if (m == NULL) return pdFALSE;

    TaskHandle_t self = xTaskGetCurrentTaskHandle();

    //Recursive fast-path: we already own it
    //if (m->owner == self) {
    //    m->lock_count++;
    //    return pdTRUE;
    //}

    // Fast path: try take immediately (supports recursive)
    if (xSemaphoreTake(m->sem, 0) == pdTRUE) {
        m->owner = self;
        m->owner_orig_prio = uxTaskPriorityGet(self);
        m->lock_count = 1;
        return pdTRUE;
    }

    // Semaphore busy: check owner and boost if needed
    TaskHandle_t owner = m->owner;
    UBaseType_t owner_prio;
    if (owner != NULL) {
        owner_prio = uxTaskPriorityGet(owner);
        UBaseType_t req_prio = uxTaskPriorityGet(self);
        if (owner_prio < req_prio) {
            ESP_LOGI(TAG, "Boost owner %p prio %u -> %u", owner, (unsigned)owner_prio, (unsigned)req_prio);
            vTaskPrioritySet(owner, req_prio);
            /* owner_orig_prio was saved when owner first acquired lock */
        }
    }

    if (xSemaphoreTake(m->sem, ticks_to_wait) != pdTRUE) {
       return pdFALSE;
    }

    // acquired after blocking
    m->owner = self;
    m->owner_orig_prio = owner_prio;
    m->lock_count = 1;

    return pdTRUE;
}

BaseType_t pip_mutex_unlock(pip_mutex_t *m)
{
    if (m == NULL) return pdFALSE;
    TaskHandle_t self = xTaskGetCurrentTaskHandle();
    if (m->owner != self) {
        // unlocking not-owner: error
        ESP_LOGW(TAG, "Unlock called by non-owner %p (owner %p)", self, m->owner);
        return pdFALSE;
    }

    // decrement recursive counter
    if (--m->lock_count > 0) {
        // still held logically
        return pdTRUE;
    }

    // fully releasing: restore original priority (if changed)
    UBaseType_t curr_prio = uxTaskPriorityGet(self);
    UBaseType_t orig_prio = m->owner_orig_prio;
    if (curr_prio != orig_prio) {
        ESP_LOGI(TAG, "Restore owner %p prio %u -> %u", self, (unsigned)curr_prio, (unsigned)m->owner_orig_prio);
        vTaskPrioritySet(self, m->owner_orig_prio);
    }

    m->owner = NULL;
    m->owner_orig_prio = 0;
    //m->lock_count = 0;

    // give semaphore back (unlocked)
    xSemaphoreGive(m->sem);
    return pdTRUE;
}

void pip_mutex_delete(pip_mutex_t *m)
{
    if (!m) return;
    if (m->sem) vSemaphoreDelete(m->sem);
    vPortFree(m);
}