# Data Collection Server

## Code Flow

1. **Data Reception**:
    - Listens for incoming data on the specified TCP port.
    - Handles chunked data transfers.
2. **Data Decoding**:
    - Decodes raw sensor data using `data_decoder.py`.
3. **Data Storage**:
    - Saves decoded data in CSV format for analysis.
4. **Data Analysis**:
    - Provides tools for visualization and analysis using `plotter.py`.

![Server Code Flowchart](../docs/server_flowchart.png)

## Configurable Settings

-   `serverPort`: TCP port for communication (default: 8889).
-   `httpPort`: HTTP port for data transmission (default: 8888).
-   Data storage paths: Modify in `server.py` if needed.

## References

-   [Main README](../README.MD)
-   Key Scripts:
    -   [`server.py`](./server.py): Main server script.
    -   [`data_decoder.py`](./data_decoder.py): Decodes incoming data.
