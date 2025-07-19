// job_state.h
#pragma once

#include <Arduino.h>
#include "../sensor/sensor_state.h"
#include "utils/others.h"
#include "data/remote_datasource.h"

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
    void begin() { reset(); }

    void startJob()
    {
        active = true;
        reset();
    }

    void reset()
    {
        index = 0;
        minute = 0;
        memset(bpmBuffer, 0, sizeof(bpmBuffer));
        memset(tempBuffer, 0, sizeof(tempBuffer));
        memset(bpmAvgPerMinute, 0, sizeof(bpmAvgPerMinute));
        memset(tempAvgPerMinute, 0, sizeof(tempAvgPerMinute));
    }

    void tick(RemoteDataSource &remote)
    {
        if (!active)
            return;

        bpmBuffer[index] = sensorState.getBPM();
        tempBuffer[index] = sensorState.getTemperature();
        index++;

        if (index >= 10)
        {
            bpmAvgPerMinute[minute] = OtherUtils::getAverage(bpmBuffer, 10);
            tempAvgPerMinute[minute] = OtherUtils::getAverage(tempBuffer, 10);

            Serial.printf("[Minute %d] BPM Avg: %.2f, Temp Avg: %.2f\n", minute + 1, bpmAvgPerMinute[minute], tempAvgPerMinute[minute]);

            index = 0;
            minute++;

            if (minute >= 5)
            {
                float finalBPM = OtherUtils::getAverage(bpmAvgPerMinute, 5);
                float finalTemp = OtherUtils::getAverage(tempAvgPerMinute, 5);

                remote.sendSensorData(finalTemp, finalBPM);

                active = false;
                ESP.deepSleep(300e6); // Sleep for 5 minutes
            }
        }
    }
};

extern JobState jobState;
