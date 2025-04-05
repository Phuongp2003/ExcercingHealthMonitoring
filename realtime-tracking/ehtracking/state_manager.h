#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

// Forward declaration - declared before including this file
extern void triggerStatusUpdate();

#include "config.h"

// Global state variables
SystemState currentState = STATE_INIT;
LedPattern currentLedPattern = LED_PATTERN_ERROR;

// State transition function
void changeState(SystemState newState)
{
    // Only change state if it's different
    if (newState == currentState)
    {
        return;
    }

    // Log state transition
    Serial.printf("State transition: %d -> %d\n", currentState, newState);

    // Update state
    currentState = newState;

    // Update LED pattern based on state
    switch (newState)
    {
    case STATE_INIT:
        currentLedPattern = LED_PATTERN_ERROR;
        break;
    case STATE_IDLE:
        currentLedPattern = LED_PATTERN_IDLE;
        break;
    case STATE_COLLECTING:
        currentLedPattern = LED_PATTERN_COLLECTING;
        break;
    case STATE_PROCESSING:
        currentLedPattern = LED_PATTERN_PROCESSING;
        break;
    case STATE_ERROR:
        currentLedPattern = LED_PATTERN_ERROR;
        break;
    }

    // Trigger a status update to sync with server
    triggerStatusUpdate();

    // Additional state transition logic if needed
    if (newState == STATE_ERROR)
    {
        Serial.println("ERROR: System entered error state");
    }
}

// Check if system is in a specific state
bool isInState(SystemState state)
{
    return currentState == state;
}

// Check if system is actively collecting data
bool isCollecting()
{
    // Consider both COLLECTING and PROCESSING states as "collecting"
    return (currentState == STATE_COLLECTING || currentState == STATE_PROCESSING);
}

// Check if system is processing data
bool isProcessing()
{
    return currentState == STATE_PROCESSING;
}

// Get name of state as string
String getStateName(SystemState state)
{
    switch (state)
    {
    case STATE_INIT:
        return "INIT";
    case STATE_IDLE:
        return "IDLE";
    case STATE_COLLECTING:
        return "COLLECTING";
    case STATE_PROCESSING:
        return "PROCESSING";
    case STATE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

// Get current state name
String getCurrentStateName()
{
    return getStateName(currentState);
}

// Get current state as numerical value
int getCurrentStateValue()
{
    return (int)currentState;
}

#endif // STATE_MANAGER_H
