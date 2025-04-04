#include <Ticker.h>
#include <CircularBuffer.hpp> // Changed from CircularBuffer.h per warning
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "sensor_reading.h"
#include "vital_signs.h"

#include "config.h"
#include "wifi_setup.h"
#include "sensor_setup.h"
#include "model_esp8266.h"

// TFLite globals
tflite::ErrorReporter *error_reporter = nullptr;
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

// Memory for TFLite arena
constexpr int kTensorArenaSize = 40 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

// Circular buffer for sensor data
CircularBuffer<SensorData, SENSOR_BUFFER_SIZE> sensorBuffer;

// Semaphores for task synchronization
SemaphoreHandle_t dataReadySemaphore;
SemaphoreHandle_t bufferMutex;

// Task handles
TaskHandle_t sensorTaskHandle;
TaskHandle_t processingTaskHandle;

// System state
bool systemActive = true;  // Controls task scheduling
bool isCollecting = false; // Controls data collection
bool isProcessing = false; // Indicates if processing task is active
bool modelReady = false;
bool wifiConnected = false;   // Tracks WiFi connection status
bool serverConnected = false; // Tracks server connection status
bool bufferReady = false;     // Indicates if buffer has enough samples for processing

// LED states and timing
unsigned long lastLedUpdate = 0;
int ledState = LED_OFF;
int ledBlinkCount = 0;
int ledBlinkPattern = 0; // 0=not connected, 1=connected waiting, 2=collecting, 3=processing

// Update LED indicator according to the specified patterns
void updateLED()
{
    unsigned long currentMillis = millis();

    // Update LED pattern based on system state
    if (!wifiConnected)
    {
        ledBlinkPattern = 0; // Not connected: 1 blink every 2 seconds
    }
    else if (wifiConnected && !isCollecting)
    {
        ledBlinkPattern = 1; // Connected, waiting: 2 blinks every 2 seconds
    }
    else if (wifiConnected && isCollecting && !isProcessing)
    {
        ledBlinkPattern = 2; // Connected, collecting: always on, blink off 2 times
    }
    else if (isProcessing)
    {
        ledBlinkPattern = 3; // Processing: 3 blinks off every 2 seconds
    }

    // Handle LED blinking based on pattern
    switch (ledBlinkPattern)
    {
    case 0: // Not connected: 1 blink 50ms/50ms every 2 seconds
        if (currentMillis - lastLedUpdate >= 2000)
        { // Every 2 seconds
            lastLedUpdate = currentMillis;
            ledState = LED_ON;
            digitalWrite(LED_WIFI_PIN, ledState);
            delay(50);
            ledState = LED_OFF;
            digitalWrite(LED_WIFI_PIN, ledState);
            delay(50);
            ledState = LED_ON;
            digitalWrite(LED_WIFI_PIN, ledState);
        }
        break;

    case 1: // Connected, waiting: 2 blinks every 2 seconds
        if (currentMillis - lastLedUpdate >= 2000)
        { // Every 2 seconds
            lastLedUpdate = currentMillis;
            // First blink
            ledState = LED_ON;
            digitalWrite(LED_WIFI_PIN, ledState);
            delay(50);
            ledState = LED_OFF;
            digitalWrite(LED_WIFI_PIN, ledState);
            delay(50);
            // Second blink
            ledState = LED_ON;
            digitalWrite(LED_WIFI_PIN, ledState);
            delay(50);
            ledState = LED_OFF;
            digitalWrite(LED_WIFI_PIN, ledState);
        }
        break;

    case 2: // Connected, collecting: always on, blink off 2 times every 2 seconds
        if (ledState == LED_OFF)
        {
            ledState = LED_ON;
            digitalWrite(LED_WIFI_PIN, ledState);
        }

        if (currentMillis - lastLedUpdate >= 2000)
        { // Every 2 seconds
            lastLedUpdate = currentMillis;
            // First blink off
            ledState = LED_OFF;
            digitalWrite(LED_WIFI_PIN, ledState);
            delay(50);
            ledState = LED_ON;
            digitalWrite(LED_WIFI_PIN, ledState);
            delay(50);
            // Second blink off
            ledState = LED_OFF;
            digitalWrite(LED_WIFI_PIN, ledState);
            delay(50);
            ledState = LED_ON;
            digitalWrite(LED_WIFI_PIN, ledState);
        }
        break;

    case 3: // Processing: 3 blinks off every 2 seconds
        if (currentMillis - lastLedUpdate >= 2000)
        { // Every 2 seconds
            lastLedUpdate = currentMillis;
            // LED is normally ON during processing
            ledState = LED_ON;
            digitalWrite(LED_WIFI_PIN, ledState);

            // Three blinks off
            for (int i = 0; i < 3; i++)
            {
                ledState = LED_OFF;
                digitalWrite(LED_WIFI_PIN, ledState);
                delay(50);
                ledState = LED_ON;
                digitalWrite(LED_WIFI_PIN, ledState);
                delay(50);
            }
        }
        break;
    }
}

// Initialize TensorFlow Lite model
bool setupTFLite()
{
    // Set up logging
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    // Map the model into a usable data structure
    model = tflite::GetModel(g_model);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        TF_LITE_REPORT_ERROR(error_reporter,
                             "Model provided is schema version %d not equal "
                             "to supported version %d.",
                             model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // Pull in all the operation implementations we need
    static tflite::AllOpsResolver resolver;

    // Build an interpreter to run the model
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
        return false;
    }

    // Get pointers to the model's input and output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);

    // Check input tensor dimensions and type
    if (input->dims->size != 3 ||
        input->dims->data[1] != WINDOW_SIZE ||
        input->dims->data[2] != 2 ||
        input->type != kTfLiteFloat32)
    {
        TF_LITE_REPORT_ERROR(error_reporter,
                             "Input tensor has incorrect dimensions or type. Expected (1,%d,2) but got (%d,%d,%d)",
                             WINDOW_SIZE, input->dims->data[0], input->dims->data[1], input->dims->data[2]);
        return false;
    }

    Serial.println("TensorFlow Lite model initialized successfully");
    Serial.printf("Model input shape: %d x %d x %d\n", input->dims->data[0], input->dims->data[1], input->dims->data[2]);
    Serial.printf("Model output shape: %d x %d\n", output->dims->data[0], output->dims->data[1]);

    return true;
}

// Run inference on the TFLite model
bool runInference(float ir_data[], float red_data[], int length, InferenceResult *result)
{
    // Validate input parameters
    if (!ir_data || !red_data || !result || length != WINDOW_SIZE)
    {
        TF_LITE_REPORT_ERROR(error_reporter, "Invalid inference parameters");
        return false;
    }

    // Copy input data to model's input tensor
    for (int i = 0; i < length; i++)
    {
        // Input is expected to be in shape (1, WINDOW_SIZE, 2)
        // First channel is IR, second is RED
        input->data.f[i * 2] = ir_data[i];
        input->data.f[i * 2 + 1] = red_data[i];
    }

    // Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed!");
        return false;
    }

    // Process output
    float maxConfidence = 0.0f;
    int maxIndex = 0;

    // Find class with highest confidence
    for (int i = 0; i < output->dims->data[1]; i++)
    {
        if (output->data.f[i] > maxConfidence)
        {
            maxConfidence = output->data.f[i];
            maxIndex = i;
        }
    }

    // Set result
    result->actionClass = maxIndex;
    result->confidence = maxConfidence;

    Serial.printf("Inference result: Class=%d, Confidence=%.4f\n", maxIndex, maxConfidence);
    return true;
}

// Task 1: Sensor reading task (40Hz)
void sensorTask(void *pvParameters)
{
    Serial.println("Sensor task started");
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(SENSOR_READ_DELAY);

    // Static allocation to reduce stack usage
    static SensorData data;
    static int sampleCount = 0;
    static int newSamplesSinceLastProcessing = 0;

    while (systemActive)
    {
        // Only collect data if isCollecting flag is true
        if (isCollecting)
        {
            // Read sensor at 40Hz
            if (readSensorData(&data))
            {
                // Critical section - keep as short as possible
                if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
                {
                    sensorBuffer.push(data);
                    sampleCount++;
                    newSamplesSinceLastProcessing++;
                    xSemaphoreGive(bufferMutex);
                }

                // Only check and signal if we have enough samples and aren't already processing
                if (newSamplesSinceLastProcessing >= WINDOW_SIZE && !isProcessing)
                {
                    // Minimal serial output to avoid stack pressure
                    Serial.printf("Buffer ready (%d samples)\n", newSamplesSinceLastProcessing);

                    // Set state and reset counter in one critical section
                    if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
                    {
                        bufferReady = true;
                        newSamplesSinceLastProcessing = 0;
                        xSemaphoreGive(bufferMutex);
                    }

                    // Signal processing task
                    xSemaphoreGive(dataReadySemaphore);
                }
            }
        }
        else
        {
            // Clear buffer when not collecting - minimal operations in critical section
            if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
            {
                sensorBuffer.clear();
                sampleCount = 0;
                newSamplesSinceLastProcessing = 0;
                bufferReady = false;
                xSemaphoreGive(bufferMutex);
            }
        }

        // Wait for the next cycle - critical for task timing
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }

    vTaskDelete(NULL);
}

// Task 2: Processing task
void processingTask(void *pvParameters)
{
    Serial.println("Processing task started");

    float ir_data[WINDOW_SIZE];
    float red_data[WINDOW_SIZE];
    InferenceResult result;

    while (systemActive)
    {
        // Wait for signal that data is ready - this task blocks until signaled
        if (xSemaphoreTake(dataReadySemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Set processing flag
            isProcessing = true;

            // Only process if buffer is ready
            if (bufferReady)
            {
                Serial.println("Processing task activated - processing 100 samples");

                // Copy data from buffer with mutex protection
                xSemaphoreTake(bufferMutex, portMAX_DELAY);

                // Make sure we still have enough data
                if (sensorBuffer.size() >= WINDOW_SIZE)
                {
                    // Extract the most recent WINDOW_SIZE samples from buffer
                    for (int i = 0; i < WINDOW_SIZE; i++)
                    {
                        int index = sensorBuffer.size() - WINDOW_SIZE + i;
                        ir_data[i] = sensorBuffer[index].ir;
                        red_data[i] = sensorBuffer[index].red;
                    }

                    xSemaphoreGive(bufferMutex);

                    // Placeholder for vital signs calculation - replace with actual calculation
                    float heartRate = 75.0f; // Placeholder value
                    float spo2 = 100.0f;     // Placeholder value

                    // Actually calculate vital signs if we have the proper implementation
                    SensorData tempBuffer[WINDOW_SIZE];
                    xSemaphoreTake(bufferMutex, portMAX_DELAY);
                    for (int i = 0; i < WINDOW_SIZE; i++)
                    {
                        int index = sensorBuffer.size() - WINDOW_SIZE + i;
                        tempBuffer[i] = sensorBuffer[index];
                    }
                    xSemaphoreGive(bufferMutex);

                    calculateVitalSigns(tempBuffer, WINDOW_SIZE, &heartRate, &spo2);
                    Serial.printf("Vital signs calculated: HR=%.1f bpm, SpO2=%.1f%%\n", heartRate, spo2);

                    // Run model inference if model is ready
                    result.heartRate = heartRate;
                    result.oxygenLevel = spo2;

                    if (modelReady)
                    {
                        Serial.println("Running model inference...");
                        if (runInference(ir_data, red_data, WINDOW_SIZE, &result))
                        {
                            Serial.printf("Model prediction: Class=%d, Confidence=%.2f\n",
                                          result.actionClass, result.confidence);
                        }
                        else
                        {
                            Serial.println("Inference failed, using default action class -1");
                            result.actionClass = -1;
                            result.confidence = 0.0f;
                        }
                    }
                    else
                    {
                        Serial.println("Model not ready, using default action class -1");
                        result.actionClass = -1;
                        result.confidence = 0.0f;
                    }

                    // Send data to server
                    Serial.println("Sending data to server...");
                    bool success = sendHTTPData(result);
                    Serial.printf("Data sent %s\n", success ? "successfully" : "with error");

                    // Clear part of the buffer after processing
                    xSemaphoreTake(bufferMutex, portMAX_DELAY);
                    int retainCount = sensorBuffer.size() / 2; // Keep half the buffer
                    int removeCount = sensorBuffer.size() - retainCount;
                    for (int i = 0; i < removeCount; i++)
                    {
                        sensorBuffer.shift();
                    }
                    xSemaphoreGive(bufferMutex);

                    Serial.println("Processing completed - waiting for next data batch");
                }
                else
                {
                    xSemaphoreGive(bufferMutex);
                    Serial.println("Not enough data for processing");
                }

                // Reset buffer ready state after processing
                bufferReady = false;
            }
            else
            {
                Serial.println("Data ready semaphore triggered but buffer not ready, skipping processing");
            }

            // Reset processing flag
            isProcessing = false;
        }
    }

    vTaskDelete(NULL);
}

// Setup function
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting EH Tracking System...");
    Serial.printf("Firmware version: 1.0.0, Built: %s %s\n", __DATE__, __TIME__);

    // Initialize pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_WIFI_PIN, OUTPUT);

    // Turn off all LEDs initially
    digitalWrite(LED_PIN, LED_OFF);
    digitalWrite(LED_WIFI_PIN, LED_OFF);

    // Initialize semaphores
    dataReadySemaphore = xSemaphoreCreateBinary();
    bufferMutex = xSemaphoreCreateMutex();

    // Initialize sensor
    if (!setupSensor())
    {
        Serial.println("ERROR: Failed to initialize sensor");
        while (1)
        {
            digitalWrite(LED_PIN, LED_ON);
            delay(100);
            digitalWrite(LED_PIN, LED_OFF);
            delay(100);
        }
    }

    // Initialize TensorFlow Lite model
    modelReady = setupTFLite();
    if (modelReady)
    {
        Serial.println("Model initialized successfully");
    }
    else
    {
        Serial.println("WARNING: Model not initialized. Running without inference.");
    }

    // Initialize WiFi
    wifiConnected = setupWiFi();
    if (wifiConnected)
    {
        Serial.println("WiFi connected successfully");

        // Connect to TCP server
        serverConnected = connectToTCPServer();
        if (serverConnected)
        {
            Serial.println("Connected to TCP server successfully");
        }
        else
        {
            Serial.println("WARNING: Failed to connect to TCP server");
        }
    }
    else
    {
        Serial.println("ERROR: Failed to connect to WiFi");
    }

    // Create tasks
    xTaskCreate(
        sensorTask,
        "SensorTask",
        SENSOR_TASK_STACK_SIZE, // Increased stack size from config.h
        NULL,
        SENSOR_TASK_PRIORITY,
        &sensorTaskHandle);

    xTaskCreate(
        processingTask,
        "ProcessingTask",
        PROCESSING_TASK_STACK_SIZE,
        NULL,
        PROCESSING_TASK_PRIORITY,
        &processingTaskHandle);

    Serial.println("System initialization complete");
}

// Main loop
void loop()
{
    // Handle WiFi and server connection
    if (!wifiConnected)
    {
        // Try to reconnect WiFi periodically
        static unsigned long lastWiFiAttempt = 0;
        if (millis() - lastWiFiAttempt > 5000)
        { // Every 5 seconds
            lastWiFiAttempt = millis();
            wifiConnected = setupWiFi();
            if (wifiConnected)
            {
                Serial.println("WiFi reconnected");
            }
        }
    }
    else if (!serverConnected)
    {
        // Try to reconnect to server periodically
        static unsigned long lastServerAttempt = 0;
        if (millis() - lastServerAttempt > 5000)
        { // Every 5 seconds
            lastServerAttempt = millis();
            serverConnected = connectToTCPServer();
            if (serverConnected)
            {
                Serial.println("Server reconnected");
            }
        }
    }
    else
    {
        // Handle TCP commands when connected
        String command = checkTCPCommands();

        if (command.length() > 0)
        {
            Serial.printf("Received command: %s\n", command.c_str());

            if (command == "START")
            {
                isCollecting = true;
                bufferReady = false; // Reset buffer ready state on start
                Serial.println("Data collection started");
                sendTCPResponse("OK: Data collection started");
            }
            else if (command == "STOP")
            {
                isCollecting = false;
                bufferReady = false; // Reset buffer ready state on stop
                Serial.println("Data collection stopped");
                sendTCPResponse("OK: Data collection stopped");
            }
            else if (command == "STATUS")
            {
                String status = "OK: ";
                status += systemActive ? "ACTIVE" : "INACTIVE";
                status += ", Collecting: " + String(isCollecting ? "YES" : "NO");
                status += ", Processing: " + String(isProcessing ? "YES" : "NO");
                status += ", Buffer: " + String(sensorBuffer.size()) + "/" + String(SENSOR_BUFFER_SIZE);
                status += ", BufferReady: " + String(bufferReady ? "YES" : "NO");
                status += ", Model: " + String(modelReady ? "Ready" : "Not ready");
                Serial.printf("Sending status: %s\n", status.c_str());
                sendTCPResponse(status);
            }
            else if (command == "WELCOME")
            {
                Serial.println("Received WELCOME message");
                sendTCPResponse("OK: Hello from device");
            }
            else
            {
                Serial.printf("Unknown command: %s\n", command.c_str());
                sendTCPResponse("ERROR: Unknown command");
            }
        }

        // Check for disconnection
        if (!tcpClient.connected())
        {
            Serial.println("TCP connection lost");
            serverConnected = false;
        }
    }

    // Update LED status
    updateLED();

    // Give other tasks time to run
    delay(10);
}
