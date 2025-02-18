#ifndef SERVER_COMMUNICATION_H
#define SERVER_COMMUNICATION_H

#include "config.h" // Import particleSensor

// Gửi 1 tín hiệu mỗi (0) ms cho server
void handleServerCommunication()
{
  // Gửi tín hiệu từ cả 3 kênh đến server
  uint32_t irValue = particleSensor.getIR();
  uint32_t redValue = particleSensor.getRed();

  uint8_t buffer[1 + 2 * sizeof(uint32_t)]; // Thêm 1 byte cho ký hiệu
  size_t index = 0;

  buffer[index++] = 0xAA; // Ký hiệu đánh dấu bắt đầu dữ liệu

  memcpy(&buffer[index], &irValue, sizeof(uint32_t));
  index += sizeof(uint32_t);
  memcpy(&buffer[index], &redValue, sizeof(uint32_t));
  index += sizeof(uint32_t);

  client.write(buffer, index);
  Serial.println("Data sent to server:");
  for (size_t i = 0; i < index; i++)
  {
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  delay(50);
}

#endif
