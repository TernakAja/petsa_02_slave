// job_state.h
#pragma once

#include <Arduino.h>
#include "../sensor/sensor_state.h"
#include "../device/device_state.h"
#include "../../utils/others.h"
#include "../../data/remote_datasource.h"

class JobState
{
private:
    float bpmBuffer[50];
    float tempBuffer[50];
    int index = 0;
    int minute = 0;

    float bpmAvgPerMinute[5];
    float tempAvgPerMinute[5];

    bool active = false;

    // Add reference to sensor state and device state
    SensorState &sensorState;
    DeviceState &deviceState;

public:
    // Constructor to initialize sensor state and device state references
    JobState(SensorState &sensor, DeviceState &device) : sensorState(sensor), deviceState(device) {}

    // begin the job state
    void begin() { reset(); }

    // start the job state
    void startJob()
    {
        active = true;
        reset();
    }

    // reset the job state
    void reset()
    {
        index = 0;
        minute = 0;
        readyForSleep = false;
        memset(bpmBuffer, 0, sizeof(bpmBuffer));
        memset(tempBuffer, 0, sizeof(tempBuffer));
        memset(bpmAvgPerMinute, 0, sizeof(bpmAvgPerMinute));
        memset(tempAvgPerMinute, 0, sizeof(tempAvgPerMinute));
    }

    // tick the job state
    void tick(RemoteDataSource &remote)
    {
        if (!active)
            return;

        // Read current sensor state
        bpmBuffer[index] = sensorState.getBPM();
        tempBuffer[index] = sensorState.getTemperature();
        index++;
        Serial.printf("BPM: %.2f, Temp: %.2f\n", bpmBuffer[index - 1], tempBuffer[index - 1]);

        // Check if we have enough data for a minute
        if (index >= 50)
        {
            // Calculate average for the minute - FIX: use all 50 samples, not just 10
            bpmAvgPerMinute[minute] = OtherUtils::getAverage(bpmBuffer, 50);
            tempAvgPerMinute[minute] = OtherUtils::getAverage(tempBuffer, 50);

            Serial.printf("[Minute %d] BPM Avg: %.2f, Temp Avg: %.2f\n", minute + 1, bpmAvgPerMinute[minute], tempAvgPerMinute[minute]);

            index = 0;
            minute++;

            // Final calculation and send data
            int nOfMinute = 1;
            if (minute >= nOfMinute)
            {
                float finalBPM = OtherUtils::getAverage(bpmAvgPerMinute, nOfMinute);
                float finalTemp = OtherUtils::getAverage(tempAvgPerMinute, nOfMinute);
                Serial.printf("Final BPM: %.2f, Final Temp: %.2f\n", finalBPM, finalTemp);

                // Try to send data with timeout protection
                unsigned long startTime = millis();
                bool dataSent = false;

                // Attempt to connect and send data with 10 second timeout
                while (millis() - startTime < 10000 && !dataSent)
                {
                    if (remote.connect())
                    {
                        dataSent = remote.sendData(finalBPM, finalTemp, 98.0);
                        Serial.println("Data sent successfully");
                    }
                    else
                    {
                        Serial.println("Failed to connect, retrying...");
                        delay(1000);
                    }
                }

                if (!dataSent)
                {
                    Serial.println("Failed to send data within timeout");
                }

                // Mark as ready for deep sleep but don't call it directly from here
                active = false;
                readyForSleep = true;
                Serial.println("Data collection complete. Ready for deep sleep...");
            }
        }
    }

    // Check if ready for deep sleep
    bool isReadyForSleep() const
    {
        return readyForSleep;
    }

    // Prepare for deep sleep using DeviceState (to be called from main loop)
    void prepareForDeepSleep(RemoteDataSource &remote)
    {
        if (!readyForSleep)
            return;

        Serial.println("[JOB] Job complete, delegating sleep preparation to DeviceState...");

        // Use DeviceState to handle the deep sleep preparation
        deviceState.prepareForDeepSleep(remote);
        // This line should never be reached as ESP.deepSleep() resets the device
    }

private:
    bool readyForSleep = false;
};
