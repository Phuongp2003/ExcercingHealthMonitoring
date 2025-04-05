#ifndef VITAL_SIGNS_H
#define VITAL_SIGNS_H

#include "config.h"
#include <algorithm>
#include <cfloat>
#include <cmath>

// Memory-efficient FFT implementation
void fft(float *data, int n, float *magnitudes)
{
    // Ensure n is a power of 2
    // Bit-reversal permutation
    int i2 = n >> 1;
    int j = 0;

    // Working arrays (on heap to avoid stack overflow)
    float *real = new float[n];
    float *imag = new float[n];

    // Initialize with input data
    for (int i = 0; i < n; i++)
    {
        real[i] = data[i];
        imag[i] = 0.0f;
    }

    for (int i = 0; i < n - 1; i++)
    {
        if (i < j)
        {
            // Swap elements
            float temp_real = real[i];
            float temp_imag = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = temp_real;
            imag[j] = temp_imag;
        }

        int k = i2;
        while (k <= j)
        {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    // Compute FFT
    for (int l = 2; l <= n; l <<= 1)
    {
        int l2 = l >> 1;
        float angle_factor = -2.0f * M_PI / l;

        for (int i = 0; i < n; i += l)
        {
            for (int j = 0; j < l2; j++)
            {
                int i1 = i + j;
                int i2 = i1 + l2;

                float cos_val = cos(j * angle_factor);
                float sin_val = sin(j * angle_factor);

                float temp_real = real[i2] * cos_val - imag[i2] * sin_val;
                float temp_imag = real[i2] * sin_val + imag[i2] * cos_val;

                real[i2] = real[i1] - temp_real;
                imag[i2] = imag[i1] - temp_imag;
                real[i1] += temp_real;
                imag[i1] += temp_imag;
            }
        }
    }

    // Calculate magnitudes
    for (int i = 0; i < n; i++)
    {
        magnitudes[i] = sqrt(real[i] * real[i] + imag[i] * imag[i]);
    }

    // Clean up
    delete[] real;
    delete[] imag;
}

// Optimized vital signs calculation with memory efficiency focus
void calculateVitalSigns(SensorData *data, int dataSize, float *heartRate, float *spo2)
{
    if (dataSize < 10)
    {
        *heartRate = 0;
        *spo2 = 0;
        return;
    }

    // Limit data size to a power of 2 for FFT (64 should be sufficient)
    const int fftSize = 64;
    int actualSize = std::min(dataSize, fftSize);

    // Static allocation for better memory management
    static float irValues[64];
    static float irSmoothed[64];
    static float magnitudes[64];

    // Extract IR values for heart rate calculation
    for (int i = 0; i < actualSize; i++)
    {
        irValues[i] = data[i].ir;
    }

    // Simple moving average smoothing to reduce noise
    for (int i = 0; i < actualSize; i++)
    {
        float sum = 0;
        int count = 0;
        for (int j = i - 2; j <= i + 2; j++)
        {
            if (j >= 0 && j < actualSize)
            {
                sum += irValues[j];
                count++;
            }
        }
        irSmoothed[i] = sum / count;
    }

    // Remove DC component (mean)
    float mean = 0;
    for (int i = 0; i < actualSize; i++)
    {
        mean += irSmoothed[i];
    }
    mean /= actualSize;

    for (int i = 0; i < actualSize; i++)
    {
        irSmoothed[i] -= mean;
    }

    // Apply FFT to find dominant frequency
    fft(irSmoothed, actualSize, magnitudes);

    // Find the peak in the frequency range corresponding to heart rate (0.5-3 Hz)
    float maxMagnitude = 0;
    int peakIndex = 0;

    // Start from bin 1 to skip DC component
    int startBin = 1;
    int endBin = std::min(actualSize / 2, (int)(3.0 / (SAMPLING_RATE / actualSize)) + 1);

    for (int i = startBin; i < endBin; i++)
    {
        if (magnitudes[i] > maxMagnitude)
        {
            maxMagnitude = magnitudes[i];
            peakIndex = i;
        }
    }

    // Convert frequency to BPM
    float peakFrequency = peakIndex * ((float)SAMPLING_RATE / actualSize);
    *heartRate = peakFrequency * 60.0f; // Convert Hz to BPM

    // Only use FFT result if magnitude is significant
    if (maxMagnitude < 10.0f)
    {
        // Fallback to peak counting method
        int peakCount = 0;
        float lastPeak = 0;
        float minVal = FLT_MAX;
        float maxVal = -FLT_MAX;

        // Find min/max for threshold calculation
        for (int i = 0; i < actualSize; i++)
        {
            if (irSmoothed[i] < minVal)
                minVal = irSmoothed[i];
            if (irSmoothed[i] > maxVal)
                maxVal = irSmoothed[i];
        }

        // Calculate adaptive threshold - require significant change
        float threshold = (maxVal - minVal) * 0.2;

        for (int i = 3; i < actualSize - 3; i++)
        {
            // Check if current point is higher than both neighbors
            if (irSmoothed[i] > irSmoothed[i - 1] && irSmoothed[i] > irSmoothed[i + 1])
            {
                // Found potential peak
                if (lastPeak == 0 || (irSmoothed[i] - lastPeak > threshold))
                {
                    peakCount++;
                    lastPeak = irSmoothed[i];
                }
            }
        }

        // Calculate heart rate: peaks per minute
        float measuredSeconds = (float)actualSize / SAMPLING_RATE;
        *heartRate = (peakCount * 60.0f) / measuredSeconds;

        Serial.println("Using peak counting method for heart rate");
    }
    else
    {
        Serial.println("Using FFT method for heart rate");
    }

    // Clamp heart rate to realistic bounds (40-180 bpm)
    *heartRate = std::max(40.0f, std::min(180.0f, *heartRate));

    // Simple SpO2 calculation
    float redSum = 0;
    float irSum = 0;
    float redMax = -FLT_MAX;
    float redMin = FLT_MAX;
    float irMax = -FLT_MAX;
    float irMin = FLT_MAX;

    for (int i = 0; i < actualSize; i++)
    {
        redSum += data[i].red;
        irSum += data[i].ir;

        if (data[i].red > redMax)
            redMax = data[i].red;
        if (data[i].red < redMin)
            redMin = data[i].red;
        if (data[i].ir > irMax)
            irMax = data[i].ir;
        if (data[i].ir < irMin)
            irMin = data[i].ir;
    }

    float redAvg = redSum / actualSize;
    float irAvg = irSum / actualSize;

    // Calculate perfusion index and ratio - better approach for SpO2
    float redDiff = redMax - redMin;
    float irDiff = irMax - irMin;

    // Avoid division by zero
    if (redDiff < 0.1f)
        redDiff = 0.1f;
    if (irDiff < 0.1f)
        irDiff = 0.1f;
    if (irAvg < 0.1f)
        irAvg = 0.1f;

    float pi_red = (redDiff / redAvg) * 100.0f;
    float pi_ir = (irDiff / irAvg) * 100.0f;

    // Ratio of ratios calculation
    float ratio = (pi_red / pi_ir);

    // Linear equation for SpO2 estimation
    *spo2 = -45.060f * ratio + 30.354f * ratio * ratio + 94.845f;

    // Clamp SpO2 to reasonable values (90-100%)
    *spo2 = std::max(90.0f, std::min(100.0f, *spo2));

    Serial.printf("Perfusion indices - RED: %.2f, IR: %.2f, Ratio: %.2f\n",
                  pi_red, pi_ir, ratio);
}

#endif // VITAL_SIGNS_H
