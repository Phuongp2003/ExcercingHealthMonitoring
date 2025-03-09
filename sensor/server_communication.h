#ifndef SERVER_COMMUNICATION_H
#define SERVER_COMMUNICATION_H

#include "config.h" // Import particleSensor
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TaskScheduler.h> // Include TaskScheduler library

const int dataCollectionInterval = 20; // 50Hz = 20ms interval
const int batchSize = 100;             // 1 second worth of data at 50Hz

struct SensorData
{
  uint32_t irValue;
  uint32_t redValue;
};

SensorData dataBatch[batchSize];
int dataIndex = 0;

// Function to send data to server
void sendDataToServer()
{
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  // Create JSON payload
  DynamicJsonDocument doc(1024);
  JsonArray dataArray = doc.createNestedArray("data");
  for (int i = 0; i < batchSize; i++)
  {
    JsonObject dataObject = dataArray.createNestedObject();
    dataObject["irValue"] = dataBatch[i].irValue;
    dataObject["redValue"] = dataBatch[i].redValue;
  }

  String payload;
  serializeJson(doc, payload);
  Serial.print("Payload: ");
  Serial.println(payload);

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
Task serverCommunicationTask(0, TASK_FOREVER, &sendDataToServer);

// Gửi 1 tín hiệu mỗi (0) ms cho server
void handleServerCommunication()
{
  // Collect data
  if (dataIndex < batchSize)
  {
    dataBatch[dataIndex].irValue = particleSensor.getIR();
    dataBatch[dataIndex].redValue = particleSensor.getRed();
    Serial.print("IR:");
    Serial.print(dataBatch[dataIndex].irValue);
    Serial.print(" Red:");
    Serial.println(dataBatch[dataIndex].redValue);
    dataIndex++;
    return;
  }

  // Schedule the server communication task
  serverCommunicationTask.enable();
}

#endif
