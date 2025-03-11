#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30105.h"

// WiFi credentials
const char *ssid = "n2heartb_oxi";
const char *password = "n2heartb_oxi";

// Server configuration
const char *serverIP = "oxinheartbeat.phuongy.works"; // Replace with your domain name
const int serverPort = 8888;

MAX30105 particleSensor;

#endif
