#pragma once

#include <Arduino.h>
#include <Wire.h>

class OtherUtils
{
public:
    void serialTimeInitialization()
    {
        Serial.begin(115200);
        delay(1000); // Give serial time to initialize
        Serial.flush();
        Wire.begin();
        Wire.setClock(400000);
    }
    static String getISOTime()
    {
        time_t now = time(nullptr);
        struct tm *timeinfo = gmtime(&now);
        char buf[25];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
        return String(buf);
    }
    void onDeviceStateChange()
    {
        if (Serial.available())
        {
            String command = Serial.readStringUntil('\n');
            command.trim(); // remove any trailing newline or space

            if (command.length() > 0)
            {
                // Forward the command to device state handler if available
                // Note: This requires DeviceState to be fully defined where this is called
                handleDeviceCommand(command);
            }
        }
    }
    
    // Static function to handle device commands - can be overridden by including DeviceState
    static void handleDeviceCommand(const String& command)
    {
        Serial.println("Device command received: " + command);
        // Basic command handling - can be extended
        if (command == "status") {
            Serial.println("Device Status: Online");
        } else if (command == "reset") {
            Serial.println("Resetting device...");
            ESP.restart();
        } else if (command == "info") {
            Serial.println("Device ID: " + getDeviceId());
            Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
            Serial.printf("Uptime: %lu ms\n", millis());
        } else {
            Serial.println("Unknown command: " + command);
            Serial.println("Available commands: status, reset, info");
        }
    }

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
        // float batteryVolt = readBatteryVoltage();
        // int batteryPercent = batteryPercentage(batteryVolt);

        Serial.printf("Temperature: %.2f °C\n", temperature);
        if (bpm > 0.0f)
        {
            Serial.printf("Heart Rate: %.2f BPM\n", bpm);
        }
        // Serial.printf("Battery: %.2f V (%d%%)\n", batteryVolt, batteryPercent);
    }
};
