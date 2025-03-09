#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "config.h" // import udpPort, serverIP, ssid, password, serverPort

// NOTE: Không sử dụng
// Truy tìm server socket bằng UDP
void discoverServer()
{
  udp.begin(udpPort);
  Serial.println("Broadcasting discovery message...");

  udp.beginPacket(IPAddress(255, 255, 255, 255), udpPort);
  udp.print("DISCOVER_SERVER");
  udp.endPacket();

  unsigned long startTime = millis();
  while (millis() - startTime < 3000)
  {
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
      char buffer[32];
      udp.read(buffer, packetSize);
      buffer[packetSize] = '\0';
      if (String(buffer).startsWith("SERVER_IP:"))
      {
        serverIP.fromString(String(buffer).substring(10));
        Serial.print("Found Server at: ");
        Serial.println(serverIP);
        break;
      }
    }
  }
  udp.stop();
}

// Xử lý kết nối wifi
void setupWiFi()
{
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  Serial.println("Connecting to server...");
  if (client.connect(serverURL, serverPort))
  {
    Serial.println("Connected to server.");
  }
  else
  {
    Serial.println("Connection to server failed.");
  }
}

#endif
