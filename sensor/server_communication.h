#ifndef SERVER_COMMUNICATION_H
#define SERVER_COMMUNICATION_H

#include "config.h"

void handleServerCommunication() {
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();
  long greenValue = particleSensor.getGreen();
  
  irBuffer[bufferIndex] = irValue;
  redBuffer[bufferIndex] = redValue;
  greenBuffer[bufferIndex] = greenValue;
  bufferIndex++;

  if (bufferIndex >= currentBufferSize) {
    String dataToSend = "";

    for (int i = 0; i < currentBufferSize; i++) {
      dataToSend += String(irBuffer[i], HEX) + "," + String(redBuffer[i], HEX) + "," + String(greenBuffer[i], HEX);
      if (i < currentBufferSize - 1) {
        dataToSend += ",";
      }
    }

    client.print(dataToSend);
    Serial.println("Data sent to server:");
    Serial.println(dataToSend);

    bufferIndex = 0;
  }

  delay(10);
}

#endif
