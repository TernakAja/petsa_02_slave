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
    float bpmBuffer[10];
    float tempBuffer[10];
    int index = 0;
    int minute = 0;

    float bpmAvgPerMinute[5];
    float tempAvgPerMinute[5];

    bool active = false;

public:
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
        // Check if we have enough data for a minute
        if (index >= 50)
        {
            // Calculate average for the minute
            bpmAvgPerMinute[minute] = OtherUtils::getAverage(bpmBuffer, 10);
            tempAvgPerMinute[minute] = OtherUtils::getAverage(tempBuffer, 10);

            Serial.printf("[Minute %d] BPM Avg: %.2f, Temp Avg: %.2f\n", minute + 1, bpmAvgPerMinute[minute], tempAvgPerMinute[minute]);

            index = 0;
            minute++; // Move to the next minute

            // Reset buffers for the next minute
            if (minute >= 5)
            {
                float finalBPM = OtherUtils::getAverage(bpmAvgPerMinute, 5);
                float finalTemp = OtherUtils::getAverage(tempAvgPerMinute, 5);

                remote.sendData(finalBPM, finalTemp, 1.0);

                active = false;
                ESP.deepSleep(300e6); // Sleep for 5 minutes
            }
        }
    }
};

extern JobState jobState;
