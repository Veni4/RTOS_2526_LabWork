#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

/*
*
* underlying binary semaphore
* current owner task handle
* owner's original priority
* recursive lock count by owner
*
*/

typedef struct pip_mutex {
    SemaphoreHandle_t sem;           
    TaskHandle_t owner;              
    UBaseType_t owner_orig_prio;    
    int lock_count;                 
} pip_mutex_t;

pip_mutex_t * pip_mutex_create(void);
BaseType_t pip_mutex_lock(pip_mutex_t *m, TickType_t ticks_to_wait);
BaseType_t pip_mutex_unlock(pip_mutex_t *m);
void pip_mutex_delete(pip_mutex_t *m);

#ifdef __cplusplus
}
#endif