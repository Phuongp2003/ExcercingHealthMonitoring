#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "MAX30105.h"

// WiFi configuration
#define WIFI_SSID "n2heartb_oxi"
#define WIFI_PASSWORD "n2heartb_oxi"

// Server configuration
#define HTTP_SERVER "192.168.137.1"
#define HTTP_PORT 8888
#define TCP_PORT 8889

// Sensor configuration
#define SAMPLING_RATE 40       // Hz
#define SENSOR_BUFFER_SIZE 250 // Number of samples to store
#define WINDOW_SIZE 100        // Size of sliding window for model input

// Memory allocation
#define PROCESSING_MEMORY_PERCENT 75
#define RECORDING_MEMORY_PERCENT 25

// Pin definitions - Using ESP32 GPIO numbers instead of Dx labels
#define MAX30102_SDA_PIN 21 // GPIO21 (SDA)
#define MAX30102_SCL_PIN 22 // GPIO22 (SCL)
#define LED_PIN 2           // GPIO2 (built-in LED on many ESP32 boards)

// Additional status LEDs
#define LED_WIFI_PIN 14       // LED for WiFi status (Blue)
#define LED_SERVER_PIN 12     // LED for server connection status (Green)
#define LED_SENSOR_PIN 27     // LED for sensor status (Yellow)
#define LED_PROCESSING_PIN 26 // LED for processing status (White)
#define LED_MODEL_PIN 25      // LED for model status (Purple)
#define LED_ERROR_PIN 33      // LED for error status (Red)

// LED states
#define LED_ON LOW // Most ESP32/ESP8266 LEDs are active LOW
#define LED_OFF HIGH

// Task priorities
#define SENSOR_TASK_PRIORITY 3
#define PROCESSING_TASK_PRIORITY 2
#define SERVER_TASK_PRIORITY 1

// Task stack sizes
#define SENSOR_TASK_STACK_SIZE 4096 // Increased from 2048
#define PROCESSING_TASK_STACK_SIZE 8192
#define SERVER_TASK_STACK_SIZE 2048

// Delay between sensor readings in ms (1000/SAMPLING_RATE)
#define SENSOR_READ_DELAY 25

// Data buffer for sensor readings
struct SensorData
{
    float red;
    float ir;
    // Add timestamp if needed
};

// Result of model inference
struct InferenceResult
{
    float heartRate;
    float oxygenLevel;
    int actionClass;
    float confidence;
};

// ESP8266 has limited task support compared to ESP32
// It's recommended to use no more than 2 tasks total on ESP8266
#define ESP8266_MAX_TASKS 2

#endif // CONFIG_H
