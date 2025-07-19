#include <Wire.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>

// Utils & Modules
#include "utils/sensors.h"
#include "utils/others.h"
#include "data/remote_datasource.h"
#include "state/sensor/sensor_state.h"

// Globals
Sensor sensor;
Ticker ticker;
RemoteDataSource remote;
OtherUtils utils;

// Setup
void setup()
{
    Serial.begin(115200);
    Wire.begin();
    Wire.setClock(400000);

    // Init sensors
    sensor.initMLX90614();
    delay(1000);
    sensor.initMAX30105();
    delay(200);

    // Init remote and timer
    remote.begin();
    ticker.attach(1.0f, []()
                  { utils.taskMaster(); }); // Every second
}

// Main Loop
void loop()
{
    sensorState.setState(
        sensor.readTemperature(),
        sensor.readHeartBeat());

    remote.loop(); // Keep MQTT alive
}
