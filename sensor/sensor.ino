#include "config.h"
#include "wifi_setup.h"
#include "sensor_setup.h"
#include "data_transmission.h"

//==== CONFIGURATION PARAMETERS ====//
// Timing and sampling rate
#define MEASURE_TIME 150000   // Maximum measurement time (150 seconds)
#define SAMPLE_INTERVAL 25    // Interval between samples (~25ms for 40Hz)
#define MAX_SAMPLES 6000      // Maximum number of samples (150s * 40Hz = 6000)
#define CONNECT_TIMEOUT 10000 // TCP server connection timeout (10 seconds)

// Chunk size for data transmission
#define CHUNK_SIZE 1000 // Samples per chunk

//==== GLOBAL VARIABLES ====//
bool isCollecting = false;            // Currently collecting data
bool isSending = false;               // Currently sending data
unsigned long startTime = 0;          // Collection start time
int sampleCount = 0;                  // Number of samples collected
unsigned long lastConnectAttempt = 0; // Last connection attempt time
bool serverResponding = false;        // Server has responded

// Array to store sensor data
SensorData measurements[MAX_SAMPLES];

// Last sample time for precise timing
unsigned long lastSampleTime = 0;

// TCP connection to server
WiFiClient client;

/**
 * Connect to TCP server
 *
 * Process:
 * 1. Check if already connected
 * 2. If not and timeout passed, attempt reconnection
 * 3. Send HELLO message when connected successfully
 */
void connectToServer()
{
  if (!client.connected())
  {
    // Wait for timeout before trying to reconnect
    if (millis() - lastConnectAttempt < CONNECT_TIMEOUT)
    {
      return;
    }
    lastConnectAttempt = millis();

    Serial.println("Connecting to server via Cloudflare Tunnel...");

    // Try to connect using the domain first
    if (client.connect(tcpServerDomain, serverPort))
    {
      Serial.println("Connected to server via domain");
      client.println("HELLO");
      serverResponding = true;
    }
    else
    {
      Serial.println("Connection failed, trying direct IP fallback...");
      // Fallback to direct IP if domain connection fails
      if (client.connect(serverIP, serverPort))
      {
        Serial.println("Connected to server via direct IP");
        client.println("HELLO");
        serverResponding = true;
      }
      else
      {
        Serial.println("All connection attempts failed");
      }
    }
  }
}

/**
 * Setup function - runs once at startup
 */
void setup()
{
  Serial.begin(115200);
  Serial.println("Starting sensor...");

  // Setup WiFi
  setupWiFi();

  // Setup sensor
  setupSensor();

  Serial.println("Setup complete");
}

/**
 * Main loop - runs continuously
 */
void loop()
{
  //==== 1. SERVER CONNECTION MANAGEMENT ====//
  if (!client.connected())
  {
    if (isCollecting)
    {
      Serial.println("Connection lost while collecting data");
      isCollecting = false;
    }
    connectToServer();
    return;
  }

  //==== 2. PROCESS SERVER COMMANDS ====//
  while (client.available())
  {
    String command = client.readStringUntil('\n');
    command.trim();
    Serial.println("Received: " + command);

    if (command.equals("START") && !isCollecting && !isSending)
    {
      Serial.println("Starting data collection");
      isCollecting = true;
      startTime = millis();
      lastSampleTime = startTime;
      sampleCount = 0;
    }
    else if (command.equals("STOP") && isCollecting)
    {
      Serial.println("Stopping. Collected: " + String(sampleCount) + " samples");
      isCollecting = false;
      isSending = true;
    }
  }

  //==== 3. DATA COLLECTION ====//
  if (isCollecting)
  {
    unsigned long currentTime = millis();

    // Check if measurement time completed
    if (currentTime - startTime >= MEASURE_TIME)
    {
      Serial.println("Measurement time completed. Samples: " + String(sampleCount));
      isCollecting = false;
      isSending = true;
    }
    // Collect samples at regular intervals
    else if ((currentTime - lastSampleTime) >= SAMPLE_INTERVAL && sampleCount < MAX_SAMPLES)
    {
      // Read sensor values
      measurements[sampleCount].timestamp = currentTime - startTime;
      measurements[sampleCount].ir = particleSensor.getIR();
      measurements[sampleCount].red = particleSensor.getRed();
      sampleCount++;
      lastSampleTime = currentTime;

      // Minimal logging - only every 1000 samples (~25 seconds at 40Hz)
      if (sampleCount % 1000 == 0)
      {
        float samplingRate = sampleCount * 1000.0 / (currentTime - startTime);
        Serial.printf("%d samples (%.1f Hz)\n", sampleCount, samplingRate);
      }
    }
  }

  //==== 4. DATA TRANSMISSION ====//
  if (isSending)
  {
    sendCollectedData(measurements, sampleCount, CHUNK_SIZE);
    isSending = false;
  }
}
