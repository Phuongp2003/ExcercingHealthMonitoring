#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "config.h"

// Forward declarations for variables defined in ehtracking.ino
extern bool systemActive;
extern bool isCollecting;
extern bool isProcessing;
extern bool modelReady;
extern bool bufferReady; // Add the new state variable
extern CircularBuffer<SensorData, SENSOR_BUFFER_SIZE> sensorBuffer;

// Function declarations
void sendTCPResponse(const String &response);

WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;

// Initialize WiFi connection
bool setupWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait up to 20 seconds for connection
  int timeout = 20;
  while (WiFi.status() != WL_CONNECTED && timeout > 0)
  {
    delay(1000);
    Serial.print(".");
    timeout--;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nFailed to connect to WiFi");
    return false;
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start TCP server
  tcpServer.begin();
  Serial.println("TCP server started");

  return true;
}

// Send response to TCP client with improved stability for critical commands
void sendTCPResponse(const String &response)
{
  if (!tcpClient.connected())
  {
    Serial.println("Cannot send response - not connected");
    return;
  }

  // Ensure string has newline at the end
  String fullResponse = response;
  if (!fullResponse.endsWith("\n"))
  {
    fullResponse += "\n";
  }

  Serial.print("Sending TCP response: ");
  Serial.println(response);

  // Use more reliable direct write method for sending data
  tcpClient.write(fullResponse.c_str(), fullResponse.length());
  tcpClient.flush();

  // Critical delay to let the network stack process before moving on
  yield();
  delay(100);
}

// Connect to TCP server with improved stability
bool connectToTCPServer()
{
  if (!tcpClient.connected())
  {
    Serial.printf("Connecting to TCP server %s:%d...\n", HTTP_SERVER, TCP_PORT);

    // Set connection timeout
    tcpClient.setTimeout(10); // 10 seconds timeout

    if (tcpClient.connect(HTTP_SERVER, TCP_PORT))
    {
      delay(100);
      Serial.println("Connected to TCP server");

      // Send HELLO message
      tcpClient.println("HELLO");
      Serial.println("Sent HELLO to server");

      // Wait for WELCOME response (with timeout)
      unsigned long startTime = millis();
      bool welcomeReceived = false;

      while (millis() - startTime < 5000 && !welcomeReceived)
      {
        if (tcpClient.available())
        {
          String response = tcpClient.readStringUntil('\n');
          response.trim();

          if (response == "WELCOME")
          {
            Serial.println("Received WELCOME from server - connection established");

            // Send status info after connection
            delay(100);
            String status = "STATUS_INFO: ACTIVE";
            status += ", Collecting: " + String(isCollecting ? "YES" : "NO");
            status += ", Processing: " + String(isProcessing ? "YES" : "NO");
            status += ", BufferReady: " + String(bufferReady ? "YES" : "NO");
            status += ", Buffer: " + String(sensorBuffer.size()) + "/" + String(SENSOR_BUFFER_SIZE);
            status += ", Model: " + String(modelReady ? "Ready" : "Not ready");

            sendTCPResponse("OK: Connection established");
            delay(200);
            sendTCPResponse(status);

            welcomeReceived = true;
          }
        }
        delay(50);
      }

      if (!welcomeReceived)
      {
        Serial.println("Never received WELCOME response");
      }

      return true;
    }
    else
    {
      Serial.println("Failed to connect to TCP server");
      return false;
    }
  }
  return true; // Already connected
}

// Send data to HTTP server
bool sendHTTPData(const InferenceResult &result)
{
  WiFiClient httpClient;
  HTTPClient http;

  String url = "http://" + String(HTTP_SERVER) + ":" + String(HTTP_PORT) + "/data";
  http.begin(httpClient, url);
  http.addHeader("Content-Type", "application/json");

  // Create JSON payload
  String payload = "{";
  payload += "\"heartRate\":" + String(result.heartRate) + ",";
  payload += "\"oxygenLevel\":" + String(result.oxygenLevel) + ",";
  payload += "\"actionClass\":" + String(result.actionClass) + ",";
  payload += "\"confidence\":" + String(result.confidence);
  payload += "}";

  // Send request
  int httpCode = http.POST(payload);

  if (httpCode > 0)
  {
    Serial.printf("HTTP response code: %d\n", httpCode);
    String response = http.getString();
    Serial.println("Response: " + response);
    http.end();
    return true;
  }
  else
  {
    Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}

// Check for TCP commands
String checkTCPCommands()
{
  String command = "";

  // Check if connected
  if (!tcpClient.connected())
  {
    return command; // Return empty string if not connected
  }

  // Check for data
  if (tcpClient.available() <= 0)
  {
    return ""; // No data available
  }

  // Read command
  command = tcpClient.readStringUntil('\n');
  command.trim();

  // Skip empty messages or responses
  if (command.length() == 0 || command.startsWith("OK:") || command.startsWith("ERROR:"))
  {
    return "";
  }

  Serial.println("Received TCP command: " + command);
  return command;
}

#endif // WIFI_SETUP_H
