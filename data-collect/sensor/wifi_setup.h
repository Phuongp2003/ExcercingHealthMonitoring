#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "config.h" // import ssid, password

// Maximum time to wait for WiFi connection (milliseconds)
#define WIFI_CONNECT_TIMEOUT 20000 // 20 seconds

/**
 * Set up WiFi connection
 *
 * Process:
 * 1. Initialize connection with credentials from config.h
 * 2. Wait until connection succeeds or times out
 * 3. Display result message and IP address if successful
 */
void setupWiFi()
{
  Serial.println("Starting WiFi connection...");
  Serial.printf("Connecting to network %s\n", ssid);

  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  // Wait for successful connection or timeout
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < WIFI_CONNECT_TIMEOUT)
  {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nWiFi connection failed. Please check network credentials.");
    // Optional: restart ESP32 after connection failure
    // ESP.restart();
  }
  else
  {
    Serial.println("\nWiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
}

#endif
