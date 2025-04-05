# ESP32 Embedded Code

## Code Flow

1. **Initialization**:
    - Configures the MAX30105 sensor for data collection.
    - Establishes a WiFi connection using credentials in `config.h`.
2. **Data Collection**:
    - Samples data from the sensor at a 40Hz rate.
    - Buffers the data and prepares it for transmission.
3. **Data Transmission**:
    - Sends data to the server in chunks defined by `CHUNK_SIZE`.

![ESP32 Code Flowchart](../docs/esp32_flowchart.png)

## Configurable Settings

-   `MEASURE_TIME`: Maximum measurement time (default: 150 seconds).
-   `SAMPLE_INTERVAL`: Sampling interval in milliseconds (default: 25ms).
-   `CHUNK_SIZE`: Number of samples per data chunk (default: 1000).

## References

-   [Main README](../README.MD)
-   Key Files:
    -   [`sensor.ino`](./sensor.ino): Main program.
    -   [`sensor_setup.h`](./sensor_setup.h): Configures the MAX30105 sensor for optimal performance.
    -   [`data_transmission.h`](./data_transmission.h): Handles data transmission to the server.

### How to Modify

-   Update WiFi credentials in `config.h`.
-   Adjust sampling parameters in `sensor.ino` as needed.
