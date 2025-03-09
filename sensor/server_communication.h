#ifndef SERVER_COMMUNICATION_H
#define SERVER_COMMUNICATION_H

#include "config.h" // Import particleSensor
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TaskScheduler.h> // Include TaskScheduler library

const int dataCollectionInterval = 50; // 20Hz = 50ms interval
const int batchSize = 40;              // 1 second worth of data at 20Hz

struct SensorData
{
  uint32_t irValue;
  uint32_t redValue;
  String timestamp;
};

SensorData dataBatch[batchSize];
int dataIndex = 0;

// Function to request server to create CSV file
bool requestServerToCreateCSV()
{
  HTTPClient http;
  http.begin(String(serverUrl) + "/create_csv");
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST("");

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
    http.end();
    return response == "CSV created";
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    Serial.println("Failed to request CSV creation");
    http.end();
    return false;
  }
}

// Function to send data to server
void sendDataToServer()
{
  HTTPClient http;
  http.begin(String(serverUrl));
  http.addHeader("Content-Type", "application/json");

  // Create compact JSON payload
  String payload = "[";
  for (int i = 0; i < batchSize; i++)
  {
    payload += "[\"" + dataBatch[i].timestamp + "\"," + String(dataBatch[i].irValue) + "," + String(dataBatch[i].redValue) + "]";
    if (i < batchSize - 1)
    {
      payload += ",";
    }
  }
  payload += "]";

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    Serial.println("Failed to send data to server");
  }

  http.end();

  Serial.println("Data batch sent to server");

  // Reset data index for next batch
  dataIndex = 0;
}

// Task to handle server communication
Task serverCommunicationTask(2000, TASK_FOREVER, &sendDataToServer);

// Gửi 1 tín hiệu mỗi (0) ms cho server
void handleServerCommunication()
{
  // Collect data
  if (dataIndex < batchSize)
  {
    dataBatch[dataIndex].irValue = particleSensor.getIR();
    dataBatch[dataIndex].redValue = particleSensor.getRed();
    dataBatch[dataIndex].timestamp = String(millis()) + "." + String(micros() % 1000); // Include milliseconds
    Serial.print("IR:");
    Serial.print(dataBatch[dataIndex].irValue);
    Serial.print(" Red:");
    Serial.println(dataBatch[dataIndex].redValue);
    dataIndex++;
    // delay(dataCollectionInterval);
    return;
  }

  serverCommunicationTask.enable();
}

#endif
