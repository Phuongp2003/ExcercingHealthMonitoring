# Embedded Health Tracking System

## Overview

This directory contains the embedded code for the ESP8266-based health tracking system. The system collects data from a MAX30105 sensor, processes it locally using a TensorFlow Lite model for activity classification, and transmits the results to a server for real-time visualization.

## System Architecture

The embedded system uses a task-based architecture with the following components:

1. **Main Loop (ehtracking.ino)**:

    - Manages system initialization
    - Maintains WiFi connectivity
    - Processes incoming commands
    - Updates status indicators

2. **Sensor Task**:

    - Collects data from the MAX30105 sensor at a configurable frequency
    - Implements double buffering for efficient data handling
    - Manages sensor configuration and calibration

3. **Processing Task**:

    - Analyzes sensor data to extract vital signs
    - Runs TensorFlow Lite model for activity classification
    - Packages and transmits processed data to the server

4. **Status Watchdog**:
    - Monitors system health and stability
    - Triggers recoveries from error states
    - Updates status information periodically

## Configuration Options

The system can be configured by modifying the parameters in the `config.h` file:

### WiFi Settings

```cpp
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASSWORD "YourPassword"
```

### Server Configuration

```cpp
#define SERVER_IP "192.168.0.100"
#define SERVER_PORT 5000
```

### Sampling Parameters

```cpp
#define SAMPLING_FREQUENCY 40        // Hz
#define SAMPLE_DURATION 4            // Seconds
#define BUFFER_SIZE (SAMPLING_FREQUENCY * SAMPLE_DURATION)
```

### Model Parameters

```cpp
#define MODEL_INPUT_SIZE 160
#define ACTIVITY_THRESHOLD 0.75      // Confidence threshold
```

## State Management

The system operates in several states, each with distinct behaviors:

-   **INIT**: System initialization
-   **IDLE**: Ready but not actively monitoring
-   **MONITORING**: Collecting and processing data
-   **PROCESSING**: Running analysis on collected data
-   **CONNECTING**: Attempting to connect to WiFi or server
-   **ERROR**: System encountered an error

Each state is visually indicated via the RGB LED:

-   Blue: Initialization or connecting
-   Green: Idle, ready for operation
-   Yellow (steady): Processing data
-   Yellow (pulsing): Actively monitoring
-   Red: Error condition

## Files and Components

-   **ehtracking.ino**: Main application file and entry point
-   **config.h**: Configuration parameters
-   **sensor_setup.h**: MAX30105 sensor initialization and configuration
-   **sensor_task.h**: Data collection task
-   **processing_task.h**: Data analysis and transmission task
-   **model_handler.h**: TensorFlow Lite model interface
-   **model_runner.h**: Activity classification execution
-   **model_esp8266.h**: Model implementation for ESP8266
-   **wifi_setup.h**: WiFi and server connection management
-   **led_control.h**: Status LED management
-   **state_manager.h**: System state tracking and transitions
-   **vital_signs.h**: Vital sign calculation algorithms
-   **status_watchdog.h**: System health monitoring
-   **manual_test.h**: Test functions for development

## Building and Uploading

1. Configure the system parameters in `config.h`
2. Open the project in Arduino IDE
3. Select the appropriate board (ESP8266 / NodeMCU)
4. Install required libraries:
    - TensorFlow Lite for Microcontrollers
    - ESP8266WiFi
    - MAX30105
5. Compile and upload to your device

## Debugging

Serial debugging information is available at 115200 baud. Connect to your device via USB to view system status and diagnostic messages.

## References

-   [Code Flow Documentation](../docs/code_flow.md)
-   [Server Documentation](../ehtrackingserver/README.md)
-   [Main Project README](../README.md)
