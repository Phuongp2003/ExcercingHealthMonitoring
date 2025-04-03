#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30105.h"

//==== WIFI CONFIGURATION ====//
// WiFi login credentials
const char *ssid = "n2heartb_oxi";     // WiFi network name
const char *password = "n2heartb_oxi"; // WiFi password

//==== SERVER CONFIGURATION ====//
// Server information
const char *tcpServerDomain = "tcp.phuongy.works";            // Domain for TCP
const char *httpServerDomain = "oxinheartbeat.phuongy.works"; // Domain for HTTP
const int serverPort = 8889;                                  // TCP port for server communication
const int httpPort = 8888;                                    // HTTP port for data transmission

// Legacy direct IP config (kept for fallback)
const char *serverIP = "192.168.137.1"; // IP address of the data receiving server

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
