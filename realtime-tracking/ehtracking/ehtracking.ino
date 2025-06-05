/*
 * EH Tracking System - Main File
 *
 * This file represents the main flow of the application, managing:
 * - System initialization
 * - Task creation
 * - WiFi connectivity
 * - State management
 * - LED status indication
 *
 * The system uses two tasks:
 * 1. Sensor Task - For data collection with double buffering
 * 2. Processing Task - For data analysis and transmission
 */

#include "config.h"

// Forward declaration of the function to resolve the circular dependency
void triggerStatusUpdate();

#include "state_manager.h"
#include "led_control.h"
#include "sensor_setup.h"
#include "sensor_task.h"
#include "processing_task.h"
#include "model_runner.h"
#include "wifi_setup.h"
#include "manual_test.h"     // Include the manual test file
#include "status_watchdog.h" // Include this after the forward declaration

// Task handles
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t processingTaskHandle = NULL;
TaskHandle_t statusWatchdogTaskHandle = NULL;

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting EH Tracking System...");
    Serial.printf("Firmware version: 1.1.0, Built: %s %s\n", __DATE__, __TIME__);

    // Initialize and test LED
    setupLED();

    // Initialize sensor hardware
    if (!setupSensor())
    {
        Serial.println("ERROR: Failed to initialize sensor");
        changeState(STATE_ERROR);
        while (1)
        {
            updateLED();
            delay(100);
        }
    }

    // Initialize data buffers and synchronization objects
    initBuffers();

    // Initialize TensorFlow Lite model
    modelInitialized = setupModel();
    if (!modelInitialized)
    {
        Serial.println("WARNING: TensorFlow model initialization failed!");
        Serial.println("The system will operate without activity classification.");
    }
    else
    {
        Serial.println("TensorFlow model initialized successfully");
        Serial.println("Activity classification is active.");

        // Only run the model test if initialization succeeded
        testModelWithSample();
    }

    // Initialize WiFi connection
    wifiConnected = setupWiFi();

    // If Wi-Fi connection successful, connect to server
    if (wifiConnected)
    {
        serverConnected = connectToTCPServer();
    }

    // Create tasks - pinned to core 0
    xTaskCreatePinnedToCore(
        sensorTask,
        "SensorTask",
        SENSOR_TASK_STACK_SIZE,
        NULL,
        SENSOR_TASK_PRIORITY,
        &sensorTaskHandle,
        0); // Pin to core 0

    xTaskCreatePinnedToCore(
        processingTask,
        "ProcessingTask",
        PROCESSING_TASK_STACK_SIZE,
        NULL,
        PROCESSING_TASK_PRIORITY,
        &processingTaskHandle,
        0); // Pin to core 0

    // Add status watchdog task
    xTaskCreatePinnedToCore(
        statusWatchdogTask,
        "StatusWatchdog",
        2048, // Smaller stack size since this is a simple task
        NULL,
        1, // Lower priority
        &statusWatchdogTaskHandle,
        0); // Pin to core 0

    // Set initial state to IDLE if everything is OK
    if (currentState == STATE_INIT)
    {
        changeState(STATE_IDLE);
    }

    // Trigger initial status update
    triggerStatusUpdate();

    Serial.println("System initialization complete");
}

void loop()
{
    // Maintain WiFi connection
    maintainWiFiConnection();
    // Maintain TCP connection
    maintainTCPConnection();

    // Check for TCP commands
    String command = checkTCPCommands();
    if (command.length() > 0)
    {
        handleTCPCommand(command);
    }

    // Update LED based on current system state
    updateLED();

    // Short delay to reduce CPU usage
    delay(10);
}
