#ifndef PROCESSING_TASK_H
#define PROCESSING_TASK_H

#include "config.h"
#include "state_manager.h"
#include "model_runner.h"
#include "vital_signs.h"
#include "wifi_setup.h"
#include "vital_signs_algo.h"

// Convert class index to activity name
const char *getActivityName(int classIndex)
{
    switch (classIndex)
    {
    case 0:
        return "resting";
    case 1:
        return "sitting";
    case 2:
        return "walking";
    default:
        return "unknown";
    }
}

// Task for processing data
void processingTask(void *parameter)
{
    Serial.println("Processing task started");

    // Result structure for output
    InferenceResult result;

    while (true)
    {
        // Wait for data to be ready for processing
        if (xSemaphoreTake(dataReadySemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Mark system as processing
            changeState(STATE_PROCESSING);
            Serial.println("Processing data...");

            // Take mutex to safely access process buffer
            if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
            {
                // Check if the processing buffer is ready
                if (processBuffer->ready && processBuffer->count >= WINDOW_SIZE)
                {
                    // Print first 5 IR and RED values from the buffer for debugging
                    Serial.println("First 5 sensor values:");
                    for (int i = 0; i < 5 && i < processBuffer->count; i++)
                    {
                        Serial.printf("  [%d] IR: %.2f, RED: %.2f\n",
                                      i, processBuffer->buffer[i].ir, processBuffer->buffer[i].red);
                    }

                    // Print last 5 IR and RED values from the buffer
                    int startIdx = max(0, processBuffer->count - 5);
                    Serial.println("Last 5 sensor values:");
                    for (int i = startIdx; i < processBuffer->count; i++)
                    {
                        Serial.printf("  [%d] IR: %.2f, RED: %.2f\n",
                                      i, processBuffer->buffer[i].ir, processBuffer->buffer[i].red);
                    }

                    xSemaphoreGive(bufferMutex);

                    // Step 1: Calculate vital signs (heart rate and SpO2)
                    float hr = 0, spo2 = 0;
                    calculateVitalSigns(processBuffer->buffer, WINDOW_SIZE,
                                        &hr, &spo2);
                    calc_spo2_filtered(processBuffer->buffer, WINDOW_SIZE, SAMPLING_RATE, &spo2);
                    result.heartRate = hr;
                    result.oxygenLevel = spo2;

                    Serial.printf("Vital signs: HR=%.1f bpm, SpO2=%.1f%%\n",
                                  result.heartRate, result.oxygenLevel);

                    // Step 2: Run model inference if model is initialized
                    if (modelInitialized)
                    {
                        if (runInference(processBuffer, &result))
                        {
                            Serial.printf("Model class: %d (%s), confidence: %.2f\n",
                                          result.actionClass,
                                          getActivityName(result.actionClass),
                                          result.confidence);
                        }
                        else
                        {
                            // If inference fails, set default values
                            result.actionClass = -1;
                            result.confidence = 0.0;
                        }
                    }
                    else
                    {
                        // If model not initialized, set default values
                        result.actionClass = -1;
                        result.confidence = 0.0;
                    }

                    // Step 3: Send data to server
                    if (isWifiConnected())
                    {
                        sendHTTPData(result);
                    }

                    // Mark processing buffer as not ready (processed)
                    if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
                    {
                        processBuffer->ready = false;
                        xSemaphoreGive(bufferMutex);
                    }
                }
                else
                {
                    Serial.println("Processing buffer not ready or insufficient data");
                    xSemaphoreGive(bufferMutex);
                }
            }

            // Return to collecting state if we were collecting
            if (isInState(STATE_PROCESSING))
            {
                changeState(STATE_COLLECTING);
            }
        }

        // Short delay to prevent tight loop if semaphore is given repeatedly
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#endif // PROCESSING_TASK_H
