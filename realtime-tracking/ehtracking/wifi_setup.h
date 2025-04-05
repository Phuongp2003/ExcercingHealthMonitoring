#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "config.h"
#include "state_manager.h"

// WiFi and server state
bool wifiConnected = false;
bool serverConnected = false;

// TCP server and client
WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;

// Initialize WiFi connection
bool setupWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait up to 10 seconds for connection
  int timeout = 10;
  while (WiFi.status() != WL_CONNECTED && timeout > 0)
  {
    delay(1000);
    Serial.print(".");
    timeout--;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nFailed to connect to WiFi");
    wifiConnected = false;
    return false;
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start TCP server
  tcpServer.begin();
  Serial.println("TCP server started");

  wifiConnected = true;

  // Update LED pattern
  if (isInState(STATE_INIT))
  {
    changeState(STATE_IDLE);
  }

  return true;
}

// Check WiFi connection status
bool isWifiConnected()
{
  return wifiConnected;
}

// Connect to TCP server
bool connectToTCPServer()
{
  if (!wifiConnected)
  {
    return false;
  }

  if (!tcpClient.connected())
  {
    Serial.printf("Connecting to TCP server %s:%d...\n", HTTP_SERVER, TCP_PORT);

    if (tcpClient.connect(HTTP_SERVER, TCP_PORT))
    {
      delay(100);
      Serial.println("Connected to TCP server");

      // Send HELLO message
      tcpClient.println("HELLO");

      // Wait for WELCOME response (with timeout)
      unsigned long startTime = millis();
      bool welcomeReceived = false;

      while (millis() - startTime < 3000 && !welcomeReceived)
      {
        if (tcpClient.available())
        {
          String response = tcpClient.readStringUntil('\n');
          response.trim();

          if (response == "WELCOME")
          {
            Serial.println("Received WELCOME from server");
            welcomeReceived = true;
          }
        }
        delay(50);
      }

      serverConnected = true;
      return true;
    }
    else
    {
      Serial.println("Failed to connect to TCP server");
      serverConnected = false;
      return false;
    }
  }

  return serverConnected;
}

// Send response to TCP client
void sendTCPResponse(const String &response)
{
  if (!tcpClient.connected())
  {
    return;
  }

  String fullResponse = response;
  if (!fullResponse.endsWith("\n"))
  {
    fullResponse += "\n";
  }

  tcpClient.write(fullResponse.c_str(), fullResponse.length());
  tcpClient.flush();

  // Allow network stack to process
  delay(50);
}

// Check for incoming TCP commands
String checkTCPCommands()
{
  String command = "";

  if (!tcpClient.connected())
  {
    // Try to accept new client if server is running
    tcpClient = tcpServer.available();
    return command;
  }

  if (tcpClient.available() <= 0)
  {
    return command;
  }

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

// Process TCP commands
void handleTCPCommand(const String &command)
{
  if (command == "START")
  {
    if (!isCollecting())
    {
      changeState(STATE_COLLECTING);
      sendTCPResponse("OK: Data collection started, State: " + getCurrentStateName());
    }
    else
    {
      sendTCPResponse("OK: Already collecting, State: " + getCurrentStateName());
    }
  }
  else if (command == "STOP")
  {
    if (isCollecting())
    {
      changeState(STATE_IDLE);
      sendTCPResponse("OK: Data collection stopped, State: " + getCurrentStateName());
    }
    else
    {
      sendTCPResponse("OK: Already stopped, State: " + getCurrentStateName());
    }
  }
  else if (command == "STATUS")
  {
    String status = "OK: ";
    status += isCollecting() ? "COLLECTING" : "IDLE";
    status += ", Processing: " + String(isProcessing() ? "YES" : "NO");
    status += ", Current State: " + getCurrentStateName();
    status += ", State Value: " + String(getCurrentStateValue());
    status += ", LED Configured: " + String((LED_ON == LOW) ? "Active LOW" : "Active HIGH");
    sendTCPResponse(status);
  }
  else if (command == "STATES")
  {
    // Send list of all available states
    String statesList = "OK: Available states: ";
    statesList += "INIT=" + String(STATE_INIT) + ", ";
    statesList += "IDLE=" + String(STATE_IDLE) + ", ";
    statesList += "COLLECTING=" + String(STATE_COLLECTING) + ", ";
    statesList += "PROCESSING=" + String(STATE_PROCESSING) + ", ";
    statesList += "ERROR=" + String(STATE_ERROR);
    sendTCPResponse(statesList);
  }
  else if (command == "LED_TEST")
  {
    // Test LED to help debug
    Serial.println("Running LED test from remote command");
    for (int i = 0; i < 5; i++)
    {
      digitalWrite(BLUE_LED_PIN, LED_ON);
      delay(200);
      digitalWrite(BLUE_LED_PIN, LED_OFF);
      delay(200);
    }
    sendTCPResponse("OK: LED test complete");
  }
  else if (command == "WELCOME")
  {
    sendTCPResponse("OK: Hello from device");
  }
  else
  {
    sendTCPResponse("ERROR: Unknown command");
  }
}

// Send data to HTTP server with state information
bool sendHTTPData(const InferenceResult &result)
{
  if (!wifiConnected)
  {
    return false;
  }

  WiFiClient httpClient;
  HTTPClient http;

  String url = "http://" + String(HTTP_SERVER) + ":" + String(HTTP_PORT) + "/data";
  http.begin(httpClient, url);
  http.addHeader("Content-Type", "application/json");

  // Create JSON payload with device state information
  String payload = "{";
  payload += "\"heartRate\":" + String(result.heartRate) + ",";
  payload += "\"oxygenLevel\":" + String(result.oxygenLevel) + ",";
  payload += "\"actionClass\":" + String(result.actionClass) + ",";
  payload += "\"confidence\":" + String(result.confidence) + ",";
  payload += "\"timestamp\":" + String(result.timestamp) + ",";
  payload += "\"deviceState\":\"" + getCurrentStateName() + "\",";
  payload += "\"isCollecting\":" + String(isCollecting() ? "true" : "false") + ",";
  payload += "\"isProcessing\":" + String(isProcessing() ? "true" : "false");
  payload += "}";

  // Send request
  int httpCode = http.POST(payload);

  if (httpCode > 0)
  {
    Serial.printf("HTTP response code: %d\n", httpCode);
    http.end();
    return true;
  }
  else
  {
    Serial.printf("HTTP POST failed, error: %s\n",
                  http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}

// Check and maintain WiFi connection
void maintainWiFiConnection()
{
  static unsigned long lastCheck = 0;
  unsigned long now = millis();

  // Check every 5 seconds
  if (now - lastCheck > 5000)
  {
    lastCheck = now;

    if (WiFi.status() != WL_CONNECTED)
    {
      if (wifiConnected)
      {
        Serial.println("WiFi connection lost");
        wifiConnected = false;
        serverConnected = false;

        // Update LED pattern
        if (isInState(STATE_IDLE) || isInState(STATE_COLLECTING))
        {
          currentLedPattern = LED_PATTERN_DISCONNECTED;
        }
      }

      // Try to reconnect
      setupWiFi();
    }
  }
}

#endif // WIFI_SETUP_H
