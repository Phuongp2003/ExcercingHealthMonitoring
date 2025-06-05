# Real-time Tracking Module

## Overview

This module implements the real-time health monitoring system, combining the trained model with live sensor data to provide continuous health monitoring and activity classification.

## System Components

### Hardware Component (ehtracking)

-   ESP32 microcontroller
-   MAX30105 sensor
-   WiFi connectivity

### Software Component (ehtrackingserver)

-   Flask web server
-   Real-time data processing
-   Web-based dashboard
-   Activity classification

## Setup Guide

### Hardware Setup

1. **Required Components**

    - ESP32 Development Board
    - MAX30105 Sensor
    - USB Cable
    - WiFi Network

2. **Wiring**
    ```
    MAX30105 <-> ESP32
    VIN      -> 3.3V
    GND      -> GND
    SDA      -> GPIO21
    SCL      -> GPIO22
    INT      -> GPIO23
    ```

### Software Setup

1. **ESP32 Firmware**

    ```bash
    # Install required libraries
    pip install esptool

    # Configure settings in config.h
    # - WiFi credentials
    # - Server URL
    # - Sensor parameters
    ```

2. **Server Setup**

    ```bash
    # Create virtual environment
    python -m venv venv
    source venv/bin/activate  # On Windows: venv\Scripts\activate

    # Install dependencies
    pip install -r requirements.txt

    # Configure server settings
    cp config.example.py config.py
    # Edit config.py with your settings
    ```

## Usage

### Starting the System

1. **Hardware**

    - Power on the ESP32
    - Verify WiFi connection
    - Check sensor initialization

2. **Server**

    ```bash
    # Start the server
    python app.py

    # Access the dashboard
    # Default: http://localhost:5000
    ```

### Dashboard Features

1. **Real-time Monitoring**

    - Heart rate graph
    - SpO2 level display
    - Activity classification
    - Historical data view

2. **Alerts and Notifications**

    - Abnormal heart rate alerts
    - Low SpO2 warnings
    - Activity change notifications

3. **Data Export**
    - CSV export
    - PDF reports
    - API access

## API Documentation

### Endpoints

1. **Real-time Data**

    ```
    GET /api/realtime
    Response: {
        "heart_rate": 75,
        "spo2": 98,
        "activity": "walking",
        "timestamp": "2024-04-07T12:00:00Z"
    }
    ```

2. **Historical Data**

    ```
    GET /api/history?start=2024-04-01&end=2024-04-07
    Response: Array of data points
    ```

3. **Activity Classification**
    ```
    POST /api/classify
    Request: Sensor data
    Response: Activity classification
    ```

## Troubleshooting

### Common Issues

1. **Sensor Connection**

    - Check wiring
    - Verify I2C communication
    - Test sensor with example code

2. **WiFi Connection**

    - Verify credentials
    - Check network settings
    - Test connection with ping

3. **Server Issues**
    - Check port availability
    - Verify database connection
    - Monitor system resources

### Debugging Tools

1. **ESP32**

    - Serial monitor
    - LED indicators
    - Debug logs

2. **Server**
    - Log files
    - Debug mode
    - Health check endpoint

## Performance Optimization

1. **Data Processing**

    - Batch processing
    - Data compression
    - Caching

2. **Network**

    - Connection pooling
    - Data buffering
    - Error handling

3. **Resource Usage**
    - Memory management
    - CPU optimization
    - Database indexing

## Security Considerations

1. **Data Protection**

    - HTTPS encryption
    - Authentication
    - Data validation

2. **Access Control**
    - User roles
    - API keys
    - Rate limiting

## Contributing

Please follow the contribution guidelines and submit pull requests for improvements.
