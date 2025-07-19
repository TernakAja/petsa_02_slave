#pragma once

#include <Arduino.h>

class OtherUtils
{
public:
    static int getAverage(float data[], int size)
    {
        int sum = 0;
        for (int i = 0; i < size; i++)
        {
            sum += data[i];
        }
        return size > 0 ? sum / size : 0;
    }

    static float readBatteryVoltage()
    {
        int raw = analogRead(A0);                    // Analog read from voltage divider
        float vout = (raw / 1023.0) * 3.3;           // Convert ADC reading to voltage (assuming 3.3V ref)
        float vbat = vout * (220.0 + 100.0) / 100.0; // Reverse voltage divider formula
        return vbat;
    }

    static int batteryPercentage(float voltage)
    {
        float minVolt = 3.0;
        float maxVolt = 4.2;
        if (voltage >= maxVolt)
            return 100;
        if (voltage <= minVolt)
            return 0;
        return int((voltage - minVolt) / (maxVolt - minVolt) * 100);
    }

    void taskMaster()
    {
        float temperature = sensorState.getTemperature();
        float bpm = sensorState.getBPM();

        float batteryVolt = readBatteryVoltage();
        int batteryPercent = batteryPercentage(batteryVolt);

        Serial.printf("Temperature: %.2f °C\n", temperature);
        if (bpm > 0.0f)
        {
            Serial.printf("Heart Rate: %.2f BPM\n", bpm);
        }
        Serial.printf("Battery: %.2f V (%d%%)\n", batteryVolt, batteryPercent);

        remote.sendSensorData(temperature, bpm);
    }
};
