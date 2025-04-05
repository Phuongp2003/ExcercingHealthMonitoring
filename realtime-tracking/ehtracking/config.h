#ifndef CONFIG_H
#define CONFIG_H

// System libraries
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Ticker.h>
#include <CircularBuffer.hpp>
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
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

// LED Pins
#define BLUE_LED_PIN 2 // LED for system status (Blue)

// LED states - try using active HIGH instead of active LOW
// since the LED is not lighting up
#define LED_ON HIGH // Changed from LOW to HIGH
#define LED_OFF LOW // Changed from HIGH to LOW

// Task priorities
#define SENSOR_TASK_PRIORITY 3
#define PROCESSING_TASK_PRIORITY 2

// Task stack sizes
#define SENSOR_TASK_STACK_SIZE 4096
#define PROCESSING_TASK_STACK_SIZE 8192

// Delay between sensor readings in ms (1000/SAMPLING_RATE)
#define SENSOR_READ_DELAY 25

// MAX30102 sensor pins
#define MAX30102_SDA_PIN 21
#define MAX30102_SCL_PIN 22

// TFLite arena size
#define TENSOR_ARENA_SIZE (40 * 1024)

// System states
typedef enum
{
    STATE_INIT,       // Initial state
    STATE_IDLE,       // System ready but not collecting
    STATE_COLLECTING, // Collecting data
    STATE_PROCESSING, // Processing data
    STATE_ERROR       // Error state
} SystemState;

// LED patterns
typedef enum
{
    LED_PATTERN_ERROR,        // Error: rapid blink
    LED_PATTERN_DISCONNECTED, // Not connected: single blink
    LED_PATTERN_IDLE,         // Connected idle: double blink
    LED_PATTERN_COLLECTING,   // Collecting: mostly on with occasional off
    LED_PATTERN_PROCESSING    // Processing: triple blink
} LedPattern;

// Data buffer for sensor readings
struct SensorData
{
    float red; // Converted from uint32_t raw data
    float ir;  // Converted from uint32_t raw data
};

// Double buffer structure
typedef struct
{
    SensorData buffer[WINDOW_SIZE];
    int count;
    bool ready;
} DataBuffer;

// Result of model inference and vital signs
struct InferenceResult
{
    float heartRate;
    float oxygenLevel;
    int actionClass;
    float confidence;
    unsigned long timestamp;
};

#endif // CONFIG_H
