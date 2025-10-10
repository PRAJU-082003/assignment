#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stdio.h"    // For printf (works on simulator/hosted build)
#include "stdint.h"

// -------------------- Global Variables --------------------
uint8_t G_DataID = 1;
int32_t G_DataValue = 0;

// -------------------- Queue and Data Definition --------------------
typedef struct {
    uint8_t dataID;
    int32_t DataValue;
} Data_t;

QueueHandle_t Queue1;

// -------------------- Task Handles --------------------
TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;

// -------------------- ExampleTask1 --------------------
void ExampleTask1(void *pv) {
    Data_t sendData;

    TickType_t xLastWakeTime = xTaskGetTickCount(); // For precise periodic delay

    while (1) {
        // Fill the structure using global variables
        sendData.dataID = G_DataID;
        sendData.DataValue = G_DataValue;

        // Send data to queue (non-blocking)
        if (xQueueSend(Queue1, &sendData, 0) == pdPASS) {
            printf("[Task1] Sent -> dataID: %d | DataValue: %ld\n", sendData.dataID, sendData.DataValue);
        } else {
            printf("[Task1] Queue Full! Could not send data.\n");
        }

        // Maintain exact 500 ms period
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

// -------------------- ExampleTask2 --------------------
void ExampleTask2(void *pv) {
    Data_t recvData;
    UBaseType_t currentPriority = uxTaskPriorityGet(NULL);
    UBaseType_t increasedPriority = currentPriority + 2;
    BaseType_t priorityIncreased = pdFALSE;

    while (1) {
        // Wait indefinitely until data arrives
        if (xQueueReceive(Queue1, &recvData, portMAX_DELAY) == pdPASS) {
            printf("[Task2] Received -> dataID: %d | DataValue: %ld\n", recvData.dataID, recvData.DataValue);

            // --- Apply Conditions ---
            if (recvData.dataID == 0) {
                printf("[Task2] dataID == 0 → Deleting Task2\n");
                vTaskDelete(NULL);
            }
            else if (recvData.dataID == 1) {
                printf("[Task2] Processing DataValue...\n");

                if (recvData.DataValue == 0 && !priorityIncreased) {
                    vTaskPrioritySet(TaskHandle_2, increasedPriority);
                    priorityIncreased = pdTRUE;
                    printf("[Task2] Increased priority to %lu\n", (unsigned long)increasedPriority);
                }
                else if (recvData.DataValue == 1 && priorityIncreased) {
                    vTaskPrioritySet(TaskHandle_2, currentPriority);
                    priorityIncreased = pdFALSE;
                    printf("[Task2] Decreased priority to %lu\n", (unsigned long)currentPriority);
                }
                else if (recvData.DataValue == 2) {
                    printf("[Task2] DataValue == 2 → Deleting Task2\n");
                    vTaskDelete(NULL);
                }
            }
        }
    }
}

// -------------------- Main Function --------------------
int main(void) {
    // Create Queue
    Queue1 = xQueueCreate(5, sizeof(Data_t));
    if (Queue1 == NULL) {
        printf("Failed to create Queue!\n");
        while (1);
    }

    // Create Tasks
    xTaskCreate(ExampleTask1, "Task1", 200, NULL, 2, &TaskHandle_1);
    xTaskCreate(ExampleTask2, "Task2", 200, NULL, 3, &TaskHandle_2);

    // Start Scheduler
    vTaskStartScheduler();

    // Should never reach here
    for (;;) {}
}
