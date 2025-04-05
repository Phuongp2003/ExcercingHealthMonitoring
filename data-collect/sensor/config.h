#ifndef CONFIG_H
#define CONFIG_H

// Include required libraries
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30105.h"
#include <ArduinoJson.h>

//==== WIFI CONFIGURATION ====//
// WiFi login credentials
const char *ssid = "M73";          // WiFi network name
const char *password = "00000000"; // WiFi password

//==== SERVER CONFIGURATION ====//
// Server information
const char *serverIP = "192.168.38.118"; // IP address of the data receiving server
const int serverPort = 8889;             // TCP port for server communication
const int httpPort = 8888;               // HTTP port for data transmission

// MAX30105 sensor object
MAX30105 particleSensor;

//==== DATA STRUCTURE ====//
// Memory-optimized sensor data structure
struct __attribute__((packed)) SensorData
{
    uint32_t timestamp; // Relative time (ms from start)
    uint32_t ir;        // IR sensor value
    uint32_t red;       // Red light sensor value
};

#endif
