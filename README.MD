# Health Monitoring System During Physical Activity

## Table of Contents

- [Health Monitoring System During Physical Activity](#health-monitoring-system-during-physical-activity)
  - [Table of Contents](#table-of-contents)
  - [General Description](#general-description)
  - [Project Structure](#project-structure)
  - [Components](#components)
    - [Data Collection (data-collect)](#data-collection-data-collect)
    - [Model Training (trainModel)](#model-training-trainmodel)
    - [Real-time Tracking (realtime-tracking)](#real-time-tracking-realtime-tracking)

## General Description

This project develops a health monitoring system during physical activity, using sensors to collect vital data (heart rate, SpO2), classifying activities through machine learning, and displaying results in real-time.

## Project Structure

The project is organized into three main components:

1. **data-collect**: Module for collecting data from the MAX30105 sensor
2. **trainModel**: Module for training the activity classification model
3. **realtime-tracking**: Module for real-time health monitoring

## Components

### Data Collection (data-collect)

This module is responsible for collecting sensor data from ESP32 hardware and MAX30105. The collected data will be used to train the activity classification model.

-   [Detailed Documentation](./data-collect/README.MD)
-   Includes source code for both hardware and server

### Model Training (trainModel)

This module uses the collected data to train an activity classification model based on sensor data. The trained model can accurately classify the user's state (resting, sitting, walking) based on data from the MAX30105 sensor.

-   **Input**: Reflective optical sensor data (IR and Red light)
-   **Output**: Activity classification (resting, sitting, walking)
-   **Technology**: TensorFlow, Jupyter Notebook
-   **Main Files**:
    -   `train.ipynb`: Notebook containing the model training process
    -   `model_esp8266.h`: Model converted to C++ header format for embedding in ESP8266
    -   `model_int8.tflite`: Model optimized with int8 format for embedded devices
    -   `best_model.h5`: Saved Keras model with the best performance

### Real-time Tracking (realtime-tracking)

This module implements a real-time health monitoring system, using the trained model to classify activity and monitor health metrics.

-   [Detailed Documentation](./realtime-tracking/README.md)
-   Includes source code for both ESP8266 hardware and web server
