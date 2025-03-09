#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30105.h"
#include "config.h"
#include "wifi_setup.h"           // Import setupWiFi()
#include "sensor_setup.h"         // Import setupSensor()
#include "server_communication.h" // import handleServerCommunication()
#include <TaskScheduler.h>        // Include TaskScheduler library

Scheduler runner; // Initialize TaskScheduler

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

  // Request server to create CSV file
  if (requestServerToCreateCSV())
  {
    Serial.println("CSV file created on server.");
    // Add the server communication task to the scheduler
    runner.addTask(serverCommunicationTask);
  }
  else
  {
    Serial.println("Failed to create CSV file on server.");
  }
}

void loop()
{
  handleServerCommunication();
  runner.execute(); // Execute scheduled tasks
}
