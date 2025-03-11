#include <WiFi.h>
#include <Wire.h>
#include "MAX30105.h"
#include "config.h"
#include "wifi_setup.h"   // Import setupWiFi()
#include "sensor_setup.h" // Import setupSensor()

bool inSend = false;
WiFiClient client;
const char *serverIP = "192.168.1.27"; // Replace with your server IP
const int serverPort = 8888;
char dataBuffer[50]; // Pre-allocated buffer for data

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup...");
  delay(2000); // Add a delay to allow the ESP to stabilize after a reset
  Serial.println("Setting up WiFi...");
  setupWiFi();
  Serial.println("Setting up sensor...");
  setupSensor();
  Serial.println("Setup complete.");
}

void loop()
{
  if (!client.connected())
  {
    Serial.println("Connecting to server...");
    if (client.connect(serverIP, serverPort))
    {
      Serial.println("Connected to server");
    }
    else
    {
      Serial.println("Connection to server failed");
      delay(1000);
      return;
    }
  }

  if (client.available())
  {
    String command = client.readStringUntil('\n');
    command.trim();
    if (command == "START")
    {
      inSend = true;
      Serial.println("Measurement started");
    }
    else if (command == "STOP")
    {
      inSend = false;
      Serial.println("Measurement stopped");
    }
  }

  if (inSend)
  {
    // Collect and send data
    uint32_t irValue = particleSensor.getIR();
    uint32_t redValue = particleSensor.getRed();
    snprintf(dataBuffer, sizeof(dataBuffer), "%lu,%lu,%lu\n", millis(), irValue, redValue);
    client.print(dataBuffer);
    Serial.println(dataBuffer);
    delay(40); // Adjust delay as needed
  }
}
