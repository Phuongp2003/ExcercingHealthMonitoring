#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "config.h"
#include "state_manager.h"
#include "sensor_reading.h" // This header already contains readSensorData()

// Global variables for buffer management and synchronization
DataBuffer bufferA;
DataBuffer bufferB;
DataBuffer *collectBuffer; // Buffer actively being filled
DataBuffer *processBuffer; // Buffer being processed

// Semaphores and mutex for synchronization
SemaphoreHandle_t bufferMutex = NULL;
SemaphoreHandle_t dataReadySemaphore = NULL;

// Initialize buffers and synchronization primitives
void initBuffers()
{
    // Initialize buffer A
    bufferA.count = 0;
    bufferA.ready = false;

    // Initialize buffer B
    bufferB.count = 0;
    bufferB.ready = false;

    // Set initial pointers
    collectBuffer = &bufferA;
    processBuffer = &bufferB;

    // Create mutex for buffer access
    bufferMutex = xSemaphoreCreateMutex();

    // Create binary semaphore for signaling data ready
    dataReadySemaphore = xSemaphoreCreateBinary();

    Serial.println("Buffers initialized");
}

// Swap the collect and process buffers
void swapBuffers()
{
    // Take mutex to ensure exclusive access
    if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
    {
        // Swap buffer pointers
        DataBuffer *temp = collectBuffer;
        collectBuffer = processBuffer;
        processBuffer = temp;

        // Reset collection buffer count
        collectBuffer->count = 0;
        collectBuffer->ready = false;

        // Mark process buffer as ready
        processBuffer->ready = true;

        // Give back the mutex
        xSemaphoreGive(bufferMutex);

        Serial.println("Buffers swapped - data ready for processing");

        // Signal processing task that data is ready
        xSemaphoreGive(dataReadySemaphore);
    }
}

// Sensor reading task
void sensorTask(void *parameter)
{
    Serial.println("Sensor task started");

    unsigned long lastReadTime = 0;
    unsigned long readCount = 0;
    unsigned long startTime = millis();

    while (true)
    {
        unsigned long currentTime = millis();

        // Only read at specified intervals
        if (currentTime - lastReadTime >= SENSOR_READ_DELAY)
        {
            lastReadTime = currentTime;

            // Modified: Continue collecting data during both COLLECTING and PROCESSING states
            if (isCollecting() || isInState(STATE_PROCESSING))
            {
                SensorData data;

                // Read data from sensor
                if (readSensorData(&data))
                {
                    // Take mutex to safely access buffer
                    if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
                    {
                        // Add data to collection buffer if not full
                        if (collectBuffer->count < WINDOW_SIZE)
                        {
                            collectBuffer->buffer[collectBuffer->count++] = data;

                            // Print reading every 40 samples (approximately once per second)
                            if (++readCount % 40 == 0)
                            {
                                Serial.printf("Reading %lu: IR=%d, RED=%d\n",
                                              readCount, (int)data.ir, (int)data.red);
                            }

                            // If buffer is full, swap it for processing
                            if (collectBuffer->count >= WINDOW_SIZE)
                            {
                                xSemaphoreGive(bufferMutex);
                                swapBuffers();
                            }
                            else
                            {
                                xSemaphoreGive(bufferMutex);
                            }
                        }
                        else
                        {
                            xSemaphoreGive(bufferMutex);
                        }
                    }
                }
            }
        }

        // Short delay to prevent CPU hogging
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

#endif // SENSOR_TASK_H
