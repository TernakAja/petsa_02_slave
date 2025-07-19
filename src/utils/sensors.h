#pragma once

#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "MAX30105.h"
#include "heartRate.h"

struct Sensor
{
    Adafruit_MLX90614 mlx;
    MAX30105 particleSensor;
    long lastBeat = 0;
    float beatsPerMinute = 0;

    void initMLX90614()
    {
        Serial.println("Initializing MLX90614...");
        if (!mlx.begin())
        {
            Serial.println("Error connecting to MLX sensor. Check wiring.");
            while (1)
                ;
        }
        Serial.print("Emissivity = ");
        Serial.println(mlx.readEmissivity());
        Serial.println("MLX90614 Loaded!");
    }

    void initMAX30105()
    {
        Serial.println("Initializing MAX30105...");
        if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
        {
            Serial.println("MAX30105 not found. Check wiring.");
            while (1)
                ;
        }
        particleSensor.setup();
        particleSensor.setPulseAmplitudeRed(0x1F);
        particleSensor.setPulseAmplitudeGreen(0);
        Serial.println("MAX30105 Loaded!");
    }

    float readHeartBeat()
    {
        long irValue = particleSensor.getIR();
        if (irValue < 20000)
        {
            delay(100);
            return 0;
        }

        static float prevIR = 0;
        float filteredIR = irValue - 0.99 * prevIR;
        prevIR = irValue;

        if (checkForBeat(filteredIR))
        {
            unsigned long now = millis();
            unsigned long delta = now - lastBeat;
            lastBeat = now;

            beatsPerMinute = 60.0 / (delta / 1000.0);
            if (beatsPerMinute > 30 && beatsPerMinute < 100)
            {
                return beatsPerMinute;
            }
        }
        return 0;
    }

    float readTemperature()
    {
        Wire.setClock(100000); // MLX90614 prefers 100kHz
        float temp = mlx.readObjectTempC();
        Wire.setClock(400000); // Restore 400kHz for MAX30105
        return temp;
    }
};
