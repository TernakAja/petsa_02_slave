#pragma once

#include <Arduino.h>

// Forward declaration
class DeviceState;
extern DeviceState deviceState;

class OtherUtils
{
public:
    void onDeviceStateChange();
    static String getDeviceId()
    {
        return "PETSA-02-" + String(ESP.getChipId(), HEX); // or DEC for decimal
    }

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

    void taskMaster(float temperature, float bpm)
    {
        //float batteryVolt = readBatteryVoltage();
        //int batteryPercent = batteryPercentage(batteryVolt);

        Serial.printf("Temperature: %.2f °C\n", temperature);
        if (bpm > 0.0f)
        {
            Serial.printf("Heart Rate: %.2f BPM\n", bpm);
        }
        //Serial.printf("Battery: %.2f V (%d%%)\n", batteryVolt, batteryPercent);
    }
};
