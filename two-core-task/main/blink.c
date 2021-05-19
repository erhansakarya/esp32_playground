#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define PRO_CPU     0
#define APP_CPU     1

int globalVarForAppCore = 0;
int globalVarForBothCore = 0;

static SemaphoreHandle_t appCoreMutex;
static portMUX_TYPE bothCoreMutex = portMUX_INITIALIZER_UNLOCKED;

static void incrementValueUsingAppCoreTasks(char *message, int whichCore) {
    if(xSemaphoreTake(appCoreMutex, 0) == pdTRUE) {
        globalVarForAppCore++;
        xSemaphoreGive(appCoreMutex);
        printf("task %s on %d, global var for app core is: %d\n", message, whichCore, globalVarForAppCore);
    } else {
        // do something else
    }
}

static void incrementValueUsingBothCoreTasks(char *message, int whichCore) {
    portENTER_CRITICAL(&bothCoreMutex);
    globalVarForBothCore++;
    portEXIT_CRITICAL(&bothCoreMutex);
    printf("task %s on %d, global var for both core is: %d\n", message, whichCore, globalVarForBothCore);
}

static void task1(void *param) {
    for(;;) {
        incrementValueUsingBothCoreTasks("task1", xPortGetCoreID());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void task2(void *param) {
    for(;;) {
        incrementValueUsingAppCoreTasks("task2", xPortGetCoreID());
        incrementValueUsingBothCoreTasks("task2", xPortGetCoreID());
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

static void task3(void *param) {
    for(;;) {
        incrementValueUsingAppCoreTasks("task3", xPortGetCoreID());
        incrementValueUsingBothCoreTasks("task3", xPortGetCoreID());
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

static void task4(void *param) {
    for(;;) {
        incrementValueUsingBothCoreTasks("task4", xPortGetCoreID());
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    appCoreMutex = xSemaphoreCreateMutex();
    
    xTaskCreatePinnedToCore(
        task1,      /* Function to implement the task */
        "task1",    /* Name of the task */
        10000,      /* Stack size in bytes, not words */
        NULL,       /* Task input parameter */ 
        0,          /* Priority of the task */ 
        NULL,       /* Task handle. */ 
        PRO_CPU     /* Core where the task should run */
    );
    xTaskCreatePinnedToCore(
        task2,      /* Function to implement the task */
        "task2",    /* Name of the task */
        10000,      /* Stack size in bytes, not words */
        NULL,       /* Task input parameter */ 
        0,          /* Priority of the task */ 
        NULL,       /* Task handle. */ 
        APP_CPU     /* Core where the task should run */
    );
    xTaskCreatePinnedToCore(
        task3,          /* Function to implement the task */
        "task3",        /* Name of the task */
        10000,          /* Stack size in bytes, not words */
        NULL,           /* Task input parameter */ 
        0,              /* Priority of the task */ 
        NULL,           /* Task handle. */ 
        APP_CPU         /* Core where the task should run */
    );
    xTaskCreatePinnedToCore(
        task4,          /* Function to implement the task */
        "task4",        /* Name of the task */
        10000,          /* Stack size in bytes, not words */
        NULL,           /* Task input parameter */ 
        0,              /* Priority of the task */ 
        NULL,           /* Task handle. */ 
        tskNO_AFFINITY  /* Which allows the task to run on both */
    );
}
