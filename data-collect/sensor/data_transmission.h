#ifndef DATA_TRANSMISSION_H
#define DATA_TRANSMISSION_H

#include "config.h" // Import serverIP and SensorData structure

/**
 * Send collected data to server in chunks
 * 
 * Process:
 * 1. Divide data into chunks (default 1000 samples/chunk)
 * 2. For each chunk, create a new HTTP connection and send data
 * 3. Add metadata to HTTP headers for chunk identification
 * 4. Wait for server response to confirm successful transmission
 */
void sendCollectedData(SensorData *measurements, int sampleCount, int chunkSize)
{
    Serial.println("Sending data...");

    // Check if there's data to send
    if (sampleCount <= 0)
    {
        Serial.println("No data to send");
        return;
    }

    // Calculate total chunks needed
    int totalChunks = (sampleCount + chunkSize - 1) / chunkSize;
    Serial.printf("Sending %d samples in %d chunks\n", sampleCount, totalChunks);

    // Send each chunk separately
    for (int chunk = 0; chunk < totalChunks; chunk++)
    {
        // Calculate start and end indices for this chunk
        int startIdx = chunk * chunkSize;
        int endIdx = min((chunk + 1) * chunkSize, sampleCount);
        int chunkSamples = endIdx - startIdx;

        Serial.printf("Sending chunk %d/%d (%d samples)\n", chunk + 1, totalChunks, chunkSamples);

        // Create a new connection for each chunk
        WiFiClient httpClient;
        if (!httpClient.connect(serverIP, 8888))
        {
            Serial.println("HTTP connection failed for chunk");
            delay(1000); // Wait before trying the next chunk
            continue;
        }

        // Prepare data for this chunk
        String data = String(chunkSamples) + "\n";
        for (int i = startIdx; i < endIdx; i++)
        {
            data += String(measurements[i].timestamp) + "," +
                    String(measurements[i].ir) + "," +
                    String(measurements[i].red) + "\n";

            // Process ESP32 background tasks occasionally
            if ((i - startIdx) % 100 == 0)
            {
                yield();
            }
        }

        // Send HTTP request with chunk information in headers
        String httpHeader = "POST /data HTTP/1.1\r\n"
                            "Host: " + String(serverIP) + "\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " + String(data.length()) + "\r\n"
                            "X-Chunk-Index: " + String(chunk) + "\r\n"
                            "X-Total-Chunks: " + String(totalChunks) + "\r\n"
                            "X-Total-Samples: " + String(sampleCount) + "\r\n"
                            "X-Chunk-Start-Index: " + String(startIdx) + "\r\n\r\n";

        Serial.printf("Sending chunk %d/%d: %d bytes\n", chunk + 1, totalChunks, data.length());
        httpClient.print(httpHeader);
        httpClient.print(data);

        // Wait for response for each chunk
        unsigned long timeout = millis();
        bool success = false;

        while (httpClient.connected() && millis() - timeout < 5000)
        {
            String line = httpClient.readStringUntil('\n');
            if (line.indexOf("200 OK") > 0)
            {
                success = true;
                break;
            }
        }

        httpClient.stop();

        if (!success)
        {
            Serial.printf("Failed to send chunk %d/%d\n", chunk + 1, totalChunks);
            delay(1000); // Wait before trying next chunk
        }
        else
        {
            Serial.printf("Chunk %d/%d transferred successfully\n", chunk + 1, totalChunks);
        }

        delay(500); // Small delay between chunks to avoid overwhelming the server
    }

    Serial.println("All chunks sent");
}

#endif
