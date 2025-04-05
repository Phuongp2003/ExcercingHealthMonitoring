#ifndef STATUS_WATCHDOG_H
#define STATUS_WATCHDOG_H

#include "config.h"
#include "wifi_setup.h"

// Global flags for status sync
volatile bool forceStatusUpdate = false;
volatile unsigned long lastStatusUpdate = 0;

// Trigger a status update - Implementation
void triggerStatusUpdate()
{
    forceStatusUpdate = true;
}

// Task that watches system status and sends updates when needed
void statusWatchdogTask(void *parameter)
{
    Serial.println("Status watchdog task started");

    const unsigned long STATUS_UPDATE_INTERVAL = 5000; // Reduced from 15000 to 5000 ms (5 seconds)

    while (true)
    {
        bool doUpdate = false;
        unsigned long currentTime = millis();

        // Check if it's time for a periodic status update
        if (currentTime - lastStatusUpdate > STATUS_UPDATE_INTERVAL)
        {
            doUpdate = true;
        }

        // Check if an update was requested
        if (forceStatusUpdate)
        {
            doUpdate = true;
            forceStatusUpdate = false;
        }

        // Send a status update if needed and connected
        if (doUpdate && isWifiConnected() && tcpClient.connected())
        {
            Serial.println("Sending automatic status update to server");

            // Create status string with more detailed information
            String status = "STATUS_INFO: ";
            status += "Current State: " + getCurrentStateName();
            status += ", Collecting: " + String(isCollecting() ? "TRUE" : "FALSE");
            status += ", Processing: " + String(isProcessing() ? "TRUE" : "FALSE");
            status += ", Uptime: " + String(millis() / 1000) + "s";

            // Send to server
            tcpClient.println(status);
            lastStatusUpdate = currentTime;
        }

        // Sleep for a shorter while to be more responsive
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

#endif // STATUS_WATCHDOG_H
