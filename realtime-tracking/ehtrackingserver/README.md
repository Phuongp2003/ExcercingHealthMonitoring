# Real-Time Health Tracking Server

## Overview

This directory contains the server-side application for the real-time health tracking system. The server receives data from ESP8266-based devices, processes this information, and provides a web interface for real-time monitoring and visualization of vital signs and activity classification.

## Features

-   Real-time data reception and processing
-   Web-based dashboard for visualization
-   Historical data tracking and analysis
-   Command interface for remote device management
-   Data logging for offline analysis

## Architecture

The server uses a Flask-based architecture with the following components:

1. **TCP Server**:

    - Listens for incoming connections from tracking devices
    - Receives and parses JSON data packets
    - Manages connection state and error handling

2. **Data Processor**:

    - Validates and processes incoming data
    - Extracts vital signs and activity classifications
    - Maintains data history and statistics

3. **Web Interface**:

    - Provides real-time dashboard using Flask and websockets
    - Visualizes vital signs with interactive charts
    - Displays activity classification results

4. **Command Interface**:
    - Enables sending commands to connected devices
    - Provides configuration interface for tracking parameters
    - Manages device status and monitoring modes

## Configuration

The server can be configured by modifying the following parameters in the server.py file:

```python
# Server settings
HOST = '0.0.0.0'  # Listen on all interfaces
PORT = 5000       # Web interface port
TCP_PORT = 5001   # Device connection port

# Application settings
MAX_DEVICES = 10      # Maximum number of connected devices
HISTORY_LENGTH = 100  # Number of data points to retain
LOG_LEVEL = 'INFO'    # Logging verbosity
```

## Web Interface

The web interface provides the following features:

-   **Dashboard**: Overview of all connected devices
-   **Device View**: Detailed view of individual device data
-   **Charts**: Real-time charts of vital signs
-   **Activity Monitor**: Visualization of detected activities
-   **Settings**: Configuration interface for system parameters
-   **Command Panel**: Interface to send commands to devices

## Data Format

The server expects JSON data from tracking devices in the following format:

```json
{
	"device_id": "esp8266_12345",
	"timestamp": 1648756218,
	"vital_signs": {
		"heart_rate": 75,
		"spo2": 98,
		"respiratory_rate": 16
	},
	"activity": {
		"class": "walking",
		"confidence": 0.87
	},
	"metrics": {
		"battery": 92,
		"signal_quality": 85,
		"temperature": 36.5
	},
	"status": "monitoring"
}
```

## Installation and Setup

1. Create and activate a virtual environment:

    ```
    python -m venv myenv
    myenv\Scripts\activate
    ```

2. Install the required dependencies:

    ```
    pip install -r requirements.txt
    ```

3. Run the server:
    ```
    python server.py
    ```

## Access

Once the server is running, access the web interface at:

-   http://localhost:5000 (or the configured port)

## Logging

The server logs all activity to `server.log`. Log level can be adjusted in the configuration to provide more detailed debugging information if needed.

## References

-   [Code Flow Documentation](../docs/code_flow.md)
-   [Embedded Code Documentation](../ehtracking/README.md)
-   [Main Project README](../README.md)
