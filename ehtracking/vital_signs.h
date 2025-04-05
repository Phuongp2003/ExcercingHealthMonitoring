#ifndef VITAL_SIGNS_H
#define VITAL_SIGNS_H

#include "config.h"
#include <algorithm>
#include <cfloat>

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
    float irSmoothed[dataSize];
    
    for (int i = 0; i < dataSize; i++)
    {
        irValues[i] = data[i].ir;
    }
    
    // Simple moving average smoothing to reduce noise
    for (int i = 0; i < dataSize; i++) {
        float sum = 0;
        int count = 0;
        for (int j = i-2; j <= i+2; j++) {
            if (j >= 0 && j < dataSize) {
                sum += irValues[j];
                count++;
            }
        }
        irSmoothed[i] = sum / count;
    }

    // Simple peak detection with adaptive threshold
    int peakCount = 0;
    bool rising = false;
    float lastPeak = 0;
    float minVal = FLT_MAX;
    float maxVal = -FLT_MAX;
    
    // Find min/max for threshold calculation
    for (int i = 0; i < dataSize; i++) {
        if (irSmoothed[i] < minVal) minVal = irSmoothed[i];
        if (irSmoothed[i] > maxVal) maxVal = irSmoothed[i];
    }
    
    // Calculate adaptive threshold - require significant change
    float threshold = (maxVal - minVal) * 0.2;
    
    for (int i = 3; i < dataSize-3; i++)
    {
        // Check if current point is higher than both neighbors
        if (irSmoothed[i] > irSmoothed[i-1] && irSmoothed[i] > irSmoothed[i+1]) {
            // Found potential peak
            if (lastPeak == 0 || (irSmoothed[i] - lastPeak > threshold)) {
                peakCount++;
                lastPeak = irSmoothed[i];
            }
        }
    }

    // Calculate heart rate: peaks per minute
    // Assuming SAMPLING_RATE in Hz and dataSize samples
    float measuredSeconds = (float)dataSize / SAMPLING_RATE;
    *heartRate = (peakCount * 60.0f) / measuredSeconds;
    
    // Clamp heart rate to realistic bounds (40-180 bpm)
    *heartRate = std::max(40.0f, std::min(180.0f, *heartRate));
    
    // If we couldn't detect meaningful peaks, provide a reasonable default
    if (peakCount < 3) {
        *heartRate = 75.0f; // Default to a common resting heart rate
        Serial.println("WARNING: Not enough peaks detected for accurate heart rate");
    }

    // Simple SpO2 calculation
    // This is a basic approximation - real SpO2 calculation is more complex
    float redSum = 0;
    float irSum = 0;
    float redMax = -FLT_MAX;
    float redMin = FLT_MAX;
    float irMax = -FLT_MAX;
    float irMin = FLT_MAX;
    
    for (int i = 0; i < dataSize; i++)
    {
        redSum += data[i].red;
        irSum += data[i].ir;
        
        if (data[i].red > redMax) redMax = data[i].red;
        if (data[i].red < redMin) redMin = data[i].red;
        if (data[i].ir > irMax) irMax = data[i].ir;
        if (data[i].ir < irMin) irMin = data[i].ir;
    }

    float redAvg = redSum / dataSize;
    float irAvg = irSum / dataSize;
    
    // Calculate perfusion index and ratio - better approach for SpO2
    float redDiff = redMax - redMin;
    float irDiff = irMax - irMin;
    
    // Avoid division by zero
    if (redDiff < 0.1f) redDiff = 0.1f;
    if (irDiff < 0.1f) irDiff = 0.1f;
    if (irAvg < 0.1f) irAvg = 0.1f;
    
    float pi_red = (redDiff / redAvg) * 100.0f;
    float pi_ir = (irDiff / irAvg) * 100.0f;
    
    // Ratio of ratios calculation
    float ratio = (pi_red / pi_ir);
    
    // Linear equation for SpO2 estimation (replace with calibrated formula)
    // SpO2 = -45.060 * R + 30.354 * R² + 94.845
    *spo2 = -45.060f * ratio + 30.354f * ratio * ratio + 94.845f;
    
    // Clamp SpO2 to reasonable values (90-100%)
    *spo2 = std::max(90.0f, std::min(100.0f, *spo2));
    
    Serial.printf("Perfusion indices - RED: %.2f, IR: %.2f, Ratio: %.2f\n", 
                 pi_red, pi_ir, ratio);
}

#endif // VITAL_SIGNS_H
