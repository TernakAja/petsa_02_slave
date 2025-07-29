#pragma once

#include <Wire.h>
#include <Adafruit_MLX90614.h>

// Prevent I2C_BUFFER_LENGTH redefinition warning
#ifndef I2C_BUFFER_LENGTH
#define I2C_BUFFER_LENGTH BUFFER_LENGTH
#endif

#include "MAX30105.h"
#include "heartRate.h"

class MLX90614Sensor
{
private:
    Adafruit_MLX90614 mlx;
    float lastValidTemp = 38.5; // Default cattle temperature

public:
    bool begin()
    {
        Serial.println("Initializing MLX90614...");
        if (!mlx.begin())
        {
            Serial.println("Error connecting to MLX sensor. Check wiring.");
            return false;
        }
        Serial.print("Emissivity = ");
        Serial.println(mlx.readEmissivity());
        Serial.println("MLX90614 Loaded!");
        return true;
    }

    
    
    float readCoreBodyTemperature()
    {
        // Set I2C speed to 100kHz for MLX90614
        Wire.setClock(100000);

        float tEar = mlx.readObjectTempC();      // Suhu telinga
        float tAmbient = mlx.readAmbientTempC(); // Suhu lingkungan

        // Restore I2C speed to 400kHz (default for MAX30105)
        Wire.setClock(400000);

        // Silently validate sensor readings
        if (isnan(tEar) || isnan(tAmbient) || 
            tEar < -10.0 || tEar > 60.0 || 
            tAmbient < -20.0 || tAmbient > 60.0) {
            return lastValidTemp; // Return last valid temperature silently
        }

        // Estimasi suhu tubuh inti menggunakan model linear empiris
        // Contoh koefisien: T_core = 0.8 * T_ear + 0.1 * T_ambient + 5
        float tCore = 0.8 * tEar + 0.1 * tAmbient + 5;

        // Silently validate calculated core temperature
        if (tCore < 30.0 || tCore > 50.0) {
            return lastValidTemp; // Return last valid temperature silently
        }

        // Update last valid temperature
        lastValidTemp = tCore;
        
        return tCore;
    }
};

class MAX30105Sensor
{
private:
    MAX30105 particleSensor;
    long lastBeat = 0;
    float beatsPerMinute = 0;

public:
    bool begin()
    {
        Serial.println("Initializing MAX30105...");
        if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
        {
            Serial.println("MAX30105 not found. Check wiring.");
            return false;
        }
        particleSensor.setup();
        particleSensor.setPulseAmplitudeRed(0x1F);
        particleSensor.setPulseAmplitudeGreen(0);
        Serial.println("MAX30105 Loaded!");
        return true;
    }

    void setSleep()
    {
        particleSensor.shutDown();        
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

        return beatsPerMinute;
    }
};

class Sensor
{
private:
    MLX90614Sensor mlx;
    MAX30105Sensor max;

public:
    bool begin()
    {
        mlx.begin();
        delay(1000);
        return max.begin();
    }

    void setSleep()
    {
        max.setSleep();
        Serial.println("Sensors set to sleep mode.");
    }

    float readTemperature()
    {
        float temp = mlx.readCoreBodyTemperature();
        
        // Additional silent validation at sensor level
        if (temp > 100.0 || temp < 30.0) {
            // Return a reasonable default temperature silently
            return 38.5; // Normal cattle temperature
        }
        
        return temp;
    }

    float readHeartBeat()
    {
        return max.readHeartBeat();
    }
};
