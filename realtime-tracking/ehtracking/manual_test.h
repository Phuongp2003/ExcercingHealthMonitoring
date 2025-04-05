#ifndef MANUAL_TEST_H
#define MANUAL_TEST_H

#include "config.h"
#include "model_runner.h"

// Generate test data for a specific activity
void generateActivityData(DataBuffer *buffer, const char *activity)
{
    buffer->count = WINDOW_SIZE;
    buffer->ready = true;

    // Base values similar to the training data
    float baseIR = 105500.0f;
    float baseRED = 101000.0f;

    if (strcmp(activity, "resting") == 0)
    {
        // Resting: small, slow oscillations
        float ampIR = 300.0f;
        float ampRED = 250.0f;
        float freq = 1.0f; // Low frequency for resting

        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            float angle = (float)i / WINDOW_SIZE * 2 * PI * freq;
            buffer->buffer[i].ir = baseIR + ampIR * sin(angle);
            buffer->buffer[i].red = baseRED + ampRED * sin(angle + 0.2);

            // Add small random noise
            buffer->buffer[i].ir += random(-50, 50);
            buffer->buffer[i].red += random(-40, 40);
        }
    }
    else if (strcmp(activity, "sitting") == 0)
    {
        // Sitting: medium oscillations
        float ampIR = 700.0f;
        float ampRED = 600.0f;
        float freq = 2.0f; // Medium frequency for sitting

        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            float angle = (float)i / WINDOW_SIZE * 2 * PI * freq;
            buffer->buffer[i].ir = baseIR + ampIR * sin(angle);
            buffer->buffer[i].red = baseRED + ampRED * sin(angle + 0.3);

            // Add medium random noise
            buffer->buffer[i].ir += random(-100, 100);
            buffer->buffer[i].red += random(-80, 80);
        }
    }
    else if (strcmp(activity, "walking") == 0)
    {
        // Walking: large, fast oscillations
        float ampIR = 1500.0f;
        float ampRED = 1200.0f;
        float freq = 5.0f; // High frequency for walking

        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            float angle = (float)i / WINDOW_SIZE * 2 * PI * freq;
            buffer->buffer[i].ir = baseIR + ampIR * sin(angle);
            buffer->buffer[i].red = baseRED + ampRED * sin(angle + 0.5);

            // Add larger random noise
            buffer->buffer[i].ir += random(-200, 200);
            buffer->buffer[i].red += random(-150, 150);
        }
    }
}

// Function to test the model with different activity patterns
void testModelWithSample()
{
    if (!modelInitialized)
    {
        Serial.println("Cannot run test - model not initialized");
        return;
    }

    Serial.println("\n=== RUNNING MODEL TEST WITH SAMPLE DATA ===");

    const char *activities[] = {"resting", "sitting", "walking"};
    DataBuffer testBuffer;
    InferenceResult result;

    for (int i = 0; i < 3; i++)
    {
        Serial.printf("\n--- Testing %s activity pattern ---\n", activities[i]);

        // Generate data for this activity
        generateActivityData(&testBuffer, activities[i]);

        // Run inference
        if (runInference(&testBuffer, &result))
        {
            Serial.printf("Test result for %s: Class=%d, Confidence=%.4f\n",
                          activities[i], result.actionClass, result.confidence);
        }
        else
        {
            Serial.printf("Test inference failed for %s\n", activities[i]);
        }
    }

    Serial.println("\n=== MODEL TEST COMPLETE ===\n");
}

#endif // MANUAL_TEST_H
