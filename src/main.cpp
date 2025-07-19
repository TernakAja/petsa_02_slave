#include <Wire.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>

#include "utils/sensors.h"
#include "utils/others.h"
#include "data/remote_datasource.h"

// Globals
Sensor sensor;
Ticker ticker;
RemoteDataSource remote;

float bpm = 0.0f;
float temperature = 0.0f;

// Periodic Task
void taskMaster()
{
    Serial.printf("Temperature: %.2f °C\n", temperature);
    if (bpm > 0.0f)
    {
        Serial.printf("Heart Rate: %.2f BPM\n", bpm);
    }

    remote.sendSensorData(temperature, bpm);
}

// Setup Function
void setup()
{
    Wire.begin();
    Serial.begin(115200);
    Wire.setClock(400000);

    sensor.initMLX90614();
    delay(1000);
    sensor.initMAX30105();
    delay(200);

    remote.begin();

    ticker.attach(1.0f, taskMaster); // Call taskMaster every second
}

// Main Loop
void loop()
{
    bpm = sensor.readHeartBeat();
    temperature = sensor.readTemperature();
    remote.loop(); // Maintain MQTT connection
}
