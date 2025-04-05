#ifndef SENSOR_SETUP_H
#define SENSOR_SETUP_H

#include "config.h"
#include "MAX30105.h"

MAX30105 particleSensor;

/**
 * Set up MAX30105 sensor
 *
 * Detailed configuration:
 * - I2C Speed: 400kHz (I2C_SPEED_FAST) to increase communication speed
 * - Sample Average: 2 samples to balance quality and speed
 * - LED Mode: 2 (Red + IR) for SpO2 calculation
 * - Sample Rate: 400Hz (internal sampling rate, divided by sampleAverage = 200Hz actual)
 * - Pulse Width: 69μs (shorter LED pulse width for faster readings)
 * - ADC Range: 2048 (smaller measurement range for faster conversion)
 */
bool setupSensor()
{
  // Initialize sensor with high-speed I2C
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) // 400kHz I2C
  {
    Serial.println("MAX30105 not found. Check connections.");
    return false; // Return false if sensor not found
  }

  Serial.println("MAX30105 found!");

  // Reset sensor before configuring
  particleSensor.softReset();

  /* Parameter optimization for 40Hz sampling
   * ---------------------------------------
   * ledBrightness: LED brightness (0x00-0x1F, 0x1F ≈ 6.4mA)
   *   - Higher value gives stronger signal but consumes more power
   *
   * sampleAverage: Number of samples averaged (1, 2, 4, 8, 16, 32)
   *   - Lower value enables faster sampling but with more noise
   *
   * ledMode: LED mode (1 = Red, 2 = Red + IR, 3 = Red + IR + Green)
   *   - Mode 2 is sufficient for SPO2 (needs both Red and IR)
   *
   * sampleRate: Sampling rate (50, 100, 200, 400, 800, 1000, 1600, 3200Hz)
   *   - 400Hz divided by sampleAverage (2) = 200Hz actual rate
   *
   * pulseWidth: LED pulse width (69, 118, 215, 411μs)
   *   - Shorter pulse = faster reading but lower resolution
   *
   * adcRange: ADC range (2048, 4096, 8192, 16384)
   *   - Smaller range converts faster but may saturate in bright light
   */
  byte ledBrightness = 0x1F;
  byte sampleAverage = 2;
  byte ledMode = 2;
  int sampleRate = 320;
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  // Adjust pulse amplitude for Red and IR LEDs
  particleSensor.setPulseAmplitudeRed(ledBrightness); // Increase red LED signal
  particleSensor.setPulseAmplitudeIR(ledBrightness);  // Increase IR LED signal

  delay(1000); // Wait for sensor to stabilize

  // Clear initial readings for stabilization
  for (int i = 0; i < 10; i++)
  {
    particleSensor.getIR();
    particleSensor.getRed();
    delay(10);
  }

  Serial.println("Sensor ready for 40Hz sampling");
  return true;
}

#endif
