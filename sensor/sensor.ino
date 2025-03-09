#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30105.h"
#include "config.h"
#include "wifi_setup.h"           // Import setupWiFi()
#include "sensor_setup.h"         // Import setupSensor()
#include "server_communication.h" // import handleServerCommunication();

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
  if (client.connected())
  {
    handleServerCommunication();
  }
  else
  {
    Serial.println("Disconnected from server. Attempting to reconnect...");
    if (client.connect(serverURL, serverPort))
    {
      Serial.println("Reconnected to server.");
      client.print("ping");
    }
  }
}
