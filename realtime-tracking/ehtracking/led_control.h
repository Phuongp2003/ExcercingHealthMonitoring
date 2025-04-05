#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "config.h"
#include "state_manager.h"

// LED timing variables
unsigned long lastLedUpdate = 0;
int blueState = LED_OFF;

// Initialize LED pins
void setupLED()
{
    pinMode(BLUE_LED_PIN, OUTPUT);
    digitalWrite(BLUE_LED_PIN, LED_OFF);

    // Simple LED test to confirm it's working
    digitalWrite(BLUE_LED_PIN, LED_ON);
    delay(200);
    digitalWrite(BLUE_LED_PIN, LED_OFF);
    delay(200);
    digitalWrite(BLUE_LED_PIN, LED_ON);
    delay(200);
    digitalWrite(BLUE_LED_PIN, LED_OFF);

    Serial.println("LED setup complete");
}

// Update LED based on current pattern
void updateLED()
{
    unsigned long currentMillis = millis();

    // Only update every 50ms to avoid excessive updating
    if (currentMillis - lastLedUpdate < 50)
    {
        return;
    }

    // Apply LED pattern based on current state
    switch (currentLedPattern)
    {
    case LED_PATTERN_ERROR:
        // Rapid blinking (250ms on, 250ms off)
        if (currentMillis - lastLedUpdate >= 250)
        {
            lastLedUpdate = currentMillis;
            blueState = (blueState == LED_ON) ? LED_OFF : LED_ON;
            digitalWrite(BLUE_LED_PIN, blueState);
        }
        break;

    case LED_PATTERN_DISCONNECTED:
        // Single blink every 2 seconds
        if (currentMillis - lastLedUpdate >= 2000)
        {
            lastLedUpdate = currentMillis;
            // Turn on for 100ms
            digitalWrite(BLUE_LED_PIN, LED_ON);
            delay(100);
            digitalWrite(BLUE_LED_PIN, LED_OFF);
            blueState = LED_OFF;
        }
        break;

    case LED_PATTERN_IDLE:
        // Double blink every 2 seconds
        if (currentMillis - lastLedUpdate >= 2000)
        {
            lastLedUpdate = currentMillis;
            // First blink
            digitalWrite(BLUE_LED_PIN, LED_ON);
            delay(100);
            digitalWrite(BLUE_LED_PIN, LED_OFF);
            delay(100);
            // Second blink
            digitalWrite(BLUE_LED_PIN, LED_ON);
            delay(100);
            digitalWrite(BLUE_LED_PIN, LED_OFF);
            blueState = LED_OFF;
        }
        break;

    case LED_PATTERN_COLLECTING:
        // Mostly on with occasional off every 2 seconds
        if (blueState == LED_OFF)
        {
            blueState = LED_ON;
            digitalWrite(BLUE_LED_PIN, blueState);
        }

        if (currentMillis - lastLedUpdate >= 2000)
        {
            lastLedUpdate = currentMillis;
            // Brief off period
            digitalWrite(BLUE_LED_PIN, LED_OFF);
            Serial.println("LED collecting pattern: brief OFF");
            delay(100);
            digitalWrite(BLUE_LED_PIN, LED_ON);
            blueState = LED_ON;
        }
        break;

    case LED_PATTERN_PROCESSING:
        // Triple blink every 2 seconds
        if (currentMillis - lastLedUpdate >= 2000)
        {
            lastLedUpdate = currentMillis;
            // Three blinks
            for (int i = 0; i < 3; i++)
            {
                digitalWrite(BLUE_LED_PIN, LED_ON);
                delay(100);
                digitalWrite(BLUE_LED_PIN, LED_OFF);
                delay(100);
            }
            blueState = LED_OFF;
        }
        break;
    }
}

#endif // LED_CONTROL_H
