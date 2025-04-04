#ifndef VITAL_SIGNS_H
#define VITAL_SIGNS_H

#include "config.h"
#include <algorithm>

// Simple algorithm to calculate heart rate and SpO2 from red and IR data
void calculateVitalSigns(SensorData *data, int dataSize, float *heartRate, float *spo2)
{
    if (dataSize < 10)
    {
        *heartRate = 0;
        *spo2 = 0;
        return;
    }

    // Basic heart rate calculation based on peak detection
    // This is a simplified algorithm and should be replaced with a more accurate one
    float irValues[dataSize];
    for (int i = 0; i < dataSize; i++)
    {
        irValues[i] = data[i].ir;
    }

    // Simple peak detection for demonstration
    int peakCount = 0;
    bool rising = false;
    for (int i = 1; i < dataSize; i++)
    {
        if (irValues[i] > irValues[i - 1])
        {
            rising = true;
        }
        else if (rising && irValues[i] < irValues[i - 1])
        {
            peakCount++;
            rising = false;
        }
    }

    // Calculate heart rate: peaks per minute
    // Assuming SAMPLING_RATE in Hz and dataSize samples
    float measuredSeconds = (float)dataSize / SAMPLING_RATE;
    *heartRate = (peakCount * 60.0f) / measuredSeconds;

    // Simple SpO2 calculation
    // This is a basic approximation - real SpO2 calculation is more complex
    float redSum = 0;
    float irSum = 0;
    for (int i = 0; i < dataSize; i++)
    {
        redSum += data[i].red;
        irSum += data[i].ir;
    }

    float redAvg = redSum / dataSize;
    float irAvg = irSum / dataSize;

    // R value (ratio of ratios) - simplified calculation
    float ratio = redAvg / irAvg;

    // SpO2 approximation based on ratio
    // Formula: SpO2 = 110 - 25 * ratio (simplified version)
    *spo2 = 110 - 25 * ratio;

    // Clamp SpO2 to reasonable values (90-100%)
    *spo2 = std::max(90.0f, std::min(100.0f, *spo2));
}

#endif // VITAL_SIGNS_H
