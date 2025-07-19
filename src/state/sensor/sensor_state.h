#pragma once

typedef void (*StateCallback)();

class SensorState
{
private:
    float bpm = 0.0f;
    float temperature = 0.0f;
    StateCallback onChange = nullptr;

public:
    void setState(float newTemp, float newBpm)
    {
        bool hasChanged = (bpm != newBpm) || (temperature != newTemp);
        bpm = newBpm;
        temperature = newTemp;

        if (hasChanged && onChange != nullptr)
        {
            onChange();
        }
    }

    float getBPM() const { return bpm; }
    float getTemperature() const { return temperature; }

    void setListener(StateCallback callback)
    {
        onChange = callback;
    }
};

// Singleton instance
extern SensorState sensorState;
