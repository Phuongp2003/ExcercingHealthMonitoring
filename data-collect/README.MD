# Data Collection Module

## Overview

The `data-collect` module is designed to collect and process data from an ESP32-based sensor system. It consists of two main components:

1. **Embedded Code**: Runs on the ESP32 to collect sensor data and transmit it to a server.
2. **Server**: Receives, decodes, and stores the data for further analysis.

## Code Flow

For a detailed explanation of the code flow, refer to the [Code Flow Documentation](./docs/code_flow.md).

## Setup

### Hardware Requirements

-   ESP32 microcontroller
-   MAX30105 sensor
-   WiFi network

### Software Requirements

-   Arduino IDE for ESP32 code
-   Python 3.10+ for the server

## Installation

### ESP32 Code

1. Open the `sensor` folder in the Arduino IDE.
2. Install the required libraries (e.g., `MAX30105`, `WiFi`).
3. Configure WiFi credentials in `config.h`.
4. Upload the code to the ESP32.

### Server

1. Navigate to the `server` folder.
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

1. Start the server:
    ```
    python server.py
    ```
2. Power on the ESP32. It will automatically connect to the server and start transmitting data.

## References

-   [ESP32 Embedded Code](./sensor/README.MD)
-   [Data Collection Server](./server/README.MD)
