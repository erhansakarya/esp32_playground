#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_timer.h"
#include "esp_event_base.h"

static const char* TAG = "user_event_loops";

// Declarations for the event source
#define TASK_ITERATIONS_COUNT        2      // number of times the task iterates
#define TASK_PERIOD                  500    // period of the task loop in milliseconds

ESP_EVENT_DECLARE_BASE(SENSOR_EVENTS);      // declaration of the task events family

typedef enum sensorEvents{
    CONFIGURE_SENSOR_EVENT = 0,
    READ_SENSOR_EVENT
}sensorEvents;

// Event loops
esp_event_loop_handle_t sensorEventLoop;

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(SENSOR_EVENTS);

static esp_err_t sensorEventHandlerCb(sensorEvents sensorEvent) {
    switch (sensorEvent) {
    case CONFIGURE_SENSOR_EVENT:
        ESP_LOGI(TAG, "configure sensor event occured!");
        break;
    case READ_SENSOR_EVENT:
        ESP_LOGI(TAG, "read sensor event occured!");
        break;
    default:
        ESP_LOGI(TAG, "unrecognized event occured!");
        break;
    }

    return ESP_OK;
}

// function called when an event is posted to the queue
static void sensorEventHandler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    // Two types of data can be passed in to the event handler: the handler specific data and the event-specific data.
    // The handler specific data (handler_args) is a pointer to the original data, therefore, the user should ensure that
    // the memory location it points to is still valid when the handler executes.
    // The event-specific data (event_data) is a pointer to a deep copy of the original data, and is managed automatically.
    sensorEvents sensorEvent = id;

    ESP_LOGI(TAG, "handling %s:%d", base, sensorEvent);

    sensorEventHandlerCb(sensorEvent);
}

static void taskEventSource(void* args) {
    for(int iteration = 0; iteration < TASK_ITERATIONS_COUNT; iteration++) {
        esp_event_loop_handle_t loop_to_post_to = sensorEventLoop;

        ESP_LOGI(TAG, "posting %s:%d", SENSOR_EVENTS, iteration);

        ESP_ERROR_CHECK(esp_event_post_to(loop_to_post_to, SENSOR_EVENTS, iteration, NULL, 0, portMAX_DELAY));

        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD));
    }

    vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD));

    ESP_LOGI(TAG, "deleting task event source");

    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_LOGI(TAG, "setting up");

    esp_event_loop_args_t sensorEventLoopArgs = {
        .queue_size = 5,
        .task_name = "sensorTask", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };

    // Create the event loops
    ESP_ERROR_CHECK(esp_event_loop_create(&sensorEventLoopArgs, &sensorEventLoop));

    // Register the handler for task iteration event. Notice that the same handler is used for handling event on different loops.
    // The loop handle is provided as an argument in order for this example to display the loop the handler is being run on.
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register_with(
            sensorEventLoop,        // the event loop to register this handler function to 
            SENSOR_EVENTS,          // the base id of the event to register the handler for
            ESP_EVENT_ANY_ID,       // all events of a certain base
            sensorEventHandler,     // the handler function which gets called when the event is dispatched
            NULL,                   // data, aside from event data, that is passed to the handler when it is called
            NULL
            )
        );

    ESP_LOGI(TAG, "starting event source");

    // Create the event source task with the same priority as the current task
    xTaskCreate(taskEventSource, "taskEventSource", 2048, NULL, uxTaskPriorityGet(NULL), NULL);
}
