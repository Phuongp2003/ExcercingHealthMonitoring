#ifndef CONFIG_H 
#define CONFIG_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30105.h"

// WiFi credentials
const char *ssid = "n2heartb_oxi";
const char *password = "n2heartb_oxi";
WiFiClient client;
WiFiUDP udp; // UDP cho dò kết nối
const int udpPort = 4210;
IPAddress serverIP(192, 168, 137, 1); // IP default của máy tính khi phát wifi
const int serverPort = 8882;

MAX30105 particleSensor;

#endif
