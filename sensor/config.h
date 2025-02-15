#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30105.h"

const char *ssid = "n2heartb_oxi";
const char *password = "n2heartb_oxi";
WiFiClient client;
WiFiUDP udp;
const int udpPort = 4210;
IPAddress serverIP(192, 168, 1, 27);
const int serverPort = 8882;

MAX30105 particleSensor;
const int maxBufferSize = 100;
long irBuffer[maxBufferSize];
long redBuffer[maxBufferSize];
long greenBuffer[maxBufferSize];
int bufferIndex = 0;

int currentBufferSize = 100;
unsigned long lastSendTime = 0;

#endif
