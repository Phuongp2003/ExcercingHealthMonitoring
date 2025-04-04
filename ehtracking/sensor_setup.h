#ifndef SENSOR_SETUP_H
#define SENSOR_SETUP_H

#include "config.h"
#include "MAX30105.h"

MAX30105 particleSensor;

bool setupSensor()
{
  // Initialize I2C communication
  Wire.begin(MAX30102_SDA_PIN, MAX30102_SCL_PIN);
  Wire.setClock(400000); // Set I2C clock speed to 400kHz

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    return false;
  }

  // Configure sensor
  particleSensor.setup(); // Configure sensor with default settings

  // Set sample average
  particleSensor.setFIFOAverage(4); // Average 4 samples per FIFO reading

  // Set LED mode
  particleSensor.setLEDMode(2); // Red and IR LED only

  // Set ADC range
  particleSensor.setADCRange(4096); // 4096 nA

  // Set sample rate
  particleSensor.setSampleRate(400); // 400 samples per second

  // Set pulse width
  particleSensor.setPulseWidth(411); // 411 μs pulse width

  // Set LED brightness
  particleSensor.setPulseAmplitudeRed(0x1F); // 0x1F is 6.4mA
  particleSensor.setPulseAmplitudeIR(0x1F);  // 0x1F is 6.4mA

  Serial.println("MAX30105 sensor initialized successfully");
  return true;
}

#endif // SENSOR_SETUP_H
