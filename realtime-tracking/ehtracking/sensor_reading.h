#ifndef SENSOR_READING_H
#define SENSOR_READING_H

#include "config.h"
#include "MAX30105.h"

extern MAX30105 particleSensor;

// Global variables to avoid stack allocations
static unsigned long lastDebugOutput = 0;
static unsigned long lastLog = 0;

// Read sensor data from the MAX30105 sensor
bool readSensorData(SensorData *data)
{
  // Debug output for availability - keeping this for troubleshooting
  // Only print every 10 seconds to minimize stack usage from string formatting
  if (millis() - lastDebugOutput > 10000)
  {
    Serial.print("Sensor status: ");
    Serial.println(particleSensor.available());
    lastDebugOutput = millis();
  }

  // First try the normal available() method
  if (particleSensor.available())
  {
    // Regular reading path
    particleSensor.check();
    data->red = particleSensor.getRed();
    data->ir = particleSensor.getIR();
    particleSensor.nextSample();
    return true;
  }
  else
  {
    // Fallback with minimal stack usage
    particleSensor.check();

    // Get the values directly
    data->red = particleSensor.getRed();
    data->ir = particleSensor.getIR();

    // Only consider readings valid if non-zero
    if (data->red > 0 && data->ir > 0)
    {
      particleSensor.nextSample();

      // Log less frequently to reduce stack pressure
      if (millis() - lastLog > 20000)
      {
        Serial.println("Using fallback method");
        lastLog = millis();
      }

      return true;
    }
  }

  // No valid data
  return false;
}

#endif // SENSOR_READING_H
