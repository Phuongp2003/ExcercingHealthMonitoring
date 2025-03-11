#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "config.h" // import ssid, password

// Xử lý kết nối wifi
void setupWiFi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

#endif
