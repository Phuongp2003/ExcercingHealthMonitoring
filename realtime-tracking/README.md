# Real-Time Health Tracking System

## Overview

The `realtime-tracking` module provides a complete health monitoring solution using ESP8266-based embedded systems to track vital signs and classify activities in real-time. This system consists of two main components:

1. **ESP8266 Embedded System**: Embedded hardware with sensor integration that collects physiological data, performs on-device processing using TensorFlow Lite, and transmits results to a server.
2. **Monitoring Server**: A Flask-based web server that receives, processes, and visualizes the health data in real-time.

The system enables continuous monitoring of vital signs such as heart rate, SpO2, and respiratory rate, while classifying activities (sitting, walking, running) through machine learning.

## Features

-   Real-time vital signs monitoring
-   TensorFlow Lite activity classification
-   Web-based visualization dashboard
-   Low-latency wireless data transmission
-   Energy-efficient embedded processing
-   Historical data tracking

## Code Flow

For a detailed explanation of the code flow, refer to the [Code Flow Documentation](./docs/code_flow.md).

## Setup

### Hardware Requirements

-   ESP8266 microcontroller (NodeMCU or compatible)
-   MAX30105 sensor
-   RGB status LED
-   WiFi network
-   USB power supply or LiPo battery

### Software Requirements

-   Arduino IDE for ESP8266 code
-   Python 3.10+ for the server
-   Required libraries:
    -   TensorFlow Lite for Microcontrollers
    -   ESP8266WiFi
    -   MAX30105 sensor library
    -   Flask and Flask-SocketIO

## Installation

### ESP8266 Embedded Code

1. Open the `ehtracking` folder in the Arduino IDE.
2. Install the required libraries through the Arduino Library Manager.
3. Configure WiFi credentials and server settings in `config.h`.
4. Upload the code to your ESP8266 device.

### Monitoring Server

1. Navigate to the `ehtrackingserver` folder.
2. Create a virtual environment and activate it:
    ```
    python -m venv myenv
    myenv\Scripts\activate
    ```
3. Install dependencies:
    ```
    pip install -r requirements.txt
    ```

## Running the System

1. Start the monitoring server:
    ```
    cd ehtrackingserver
    python server.py
    ```
2. Power on the ESP8266 device. It will automatically:

    - Connect to the configured WiFi network
    - Establish a connection with the server
    - Begin collecting and transmitting data

3. Access the web dashboard by navigating to:
    ```
    http://localhost:5000
    ```

## System Architecture

The system follows a task-based architecture for embedded processing and a client-server model for data visualization:

```
┌─────────────────────┐      WiFi      ┌─────────────────────┐
│                     │  Transmission   │                     │
│  ESP8266 Device     │ ─────────────> │  Monitoring Server   │
│  - Sensor Reading   │                │  - Data Reception   │
│  - ML Processing    │ <─────────────┐│  - Data Processing  │
│  - Data Transmission│  Device Cmds   │  - Visualization    │
└─────────────────────┘                └─────────────────────┘
                                                │
                                                │ HTTP/WebSocket
                                                ▼
                                       ┌─────────────────────┐
                                       │                     │
                                       │  Web Dashboard      │
                                       │  - Real-time Charts │
                                       │  - Activity Display │
                                       │  - System Controls  │
                                       └─────────────────────┘
```

## References

-   [Embedded Code Documentation](./ehtracking/README.md)
-   [Server Documentation](./ehtrackingserver/README.md)
