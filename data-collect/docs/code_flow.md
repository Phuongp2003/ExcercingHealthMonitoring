# Code Flow Documentation

This document provides a detailed explanation of the code flow for the `data-collect` module, including both the ESP32 embedded code and the server.

## ESP32 Embedded Code

1. **Initialization**:
    - Configures the MAX30105 sensor for data collection.
    - Establishes a WiFi connection using credentials in `config.h`.
2. **Data Collection**:
    - Samples data from the sensor at a 40Hz rate.
    - Buffers the data and prepares it for transmission.
3. **Data Transmission**:
    - Sends data to the server in chunks defined by `CHUNK_SIZE`.

![ESP32 Code Flowchart](./images/esp32_flowchart.png)

## Server

1. **Data Reception**:
    - Listens for incoming data on the specified TCP port.
    - Handles chunked data transfers.
2. **Data Decoding**:
    - Decodes raw sensor data using `data_decoder.py`.
3. **Data Storage**:
    - Saves decoded data in CSV format for analysis.
4. **Data Analysis**:
    - Provides tools for visualization and analysis using `plotter.py`.

![Server Code Flowchart](./images/server_flowchart.png)

## References

-   [Main README](../README.MD)
-   [ESP32 Embedded Code](../sensor/README.MD)
-   [Data Collection Server](../server/README.MD)
