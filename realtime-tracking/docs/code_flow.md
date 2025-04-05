# Code Flow Documentation

This document provides a detailed explanation of the code flow for the `realtime-tracking` module, including both the ESP8266 embedded code and the server.

## ESP8266 Embedded Code (ehtracking)

### Initialization Flow

1. **System Startup**:

    - Initializes serial communication for debugging
    - Sets up the RGB LED for status indication
    - Configures the MAX30105 sensor for data collection
    - Initializes TensorFlow Lite model for activity classification
    - Establishes a WiFi connection using credentials in `config.h`
    - Connects to the TCP server for data transmission

2. **Task Creation**:

    - Creates a Sensor Task for data collection with double buffering
    - Creates a Processing Task for data analysis and transmission
    - Creates a Status Watchdog Task to monitor system health

3. **State Management**:
    - System moves from STATE_INIT to STATE_IDLE if initialization is successful
    - Updates LED color to indicate current system state

### Main Loop Flow

1. **WiFi Management**:

    - Maintains WiFi connectivity
    - Reconnects automatically if connection is lost

2. **Command Processing**:

    - Checks for incoming TCP commands from server
    - Processes commands to start/stop monitoring, change settings, etc.

3. **Status Indication**:
    - Updates LED color based on the current system state
    - Provides visual feedback of system operation

### Sensor Task Flow

1. **Data Collection**:
    - Samples data from MAX30105 sensor at configured frequency
    - Implements double buffering to prevent data loss during processing
    - Signals Processing Task when buffer is ready

### Processing Task Flow

1. **Data Analysis**:

    - Processes raw sensor data to extract vital signs
    - Runs activity classification using TensorFlow Lite model
    - Computes health metrics and status indicators

2. **Data Transmission**:
    - Packages processed data into JSON format
    - Sends data to server over TCP connection
    - Handles transmission failures and retry logic

## Server (ehtrackingserver)

1. **Data Reception**:

    - Listens for incoming connections on the specified TCP port
    - Receives JSON data packets from ESP8266 devices
    - Parses and validates incoming data

2. **Data Processing**:

    - Processes incoming vital signs data
    - Tracks activity classifications
    - Maintains historical data for visualization

3. **Web Interface**:

    - Provides real-time dashboard visualization
    - Updates charts and indicators as new data arrives
    - Displays health status and activity classification results

4. **Command Interface**:
    - Allows sending commands to connected ESP8266 devices
    - Provides configuration options for monitoring parameters
    - Enables remote control of the tracking system

## Data Flow Diagram

```
+-------------+     +---------------+     +----------------+
| MAX30105    |---->| Double Buffer |---->| Data Analysis  |
| Sensor      |     | System        |     | (TensorFlow)   |
+-------------+     +---------------+     +----------------+
                                                 |
                                                 v
+-----------------+     +---------------+     +----------------+
| Web Dashboard   |<----| Server        |<----| TCP/IP         |
| Visualization   |     | Processing    |     | Transmission   |
+-----------------+     +---------------+     +----------------+
```


## References

-   [Main README](../README.md)
-   [ESP8266 Embedded Code](../ehtracking/README.md)
-   [Server Documentation](../ehtrackingserver/README.md)
