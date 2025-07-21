#include <Wire.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>

// Utils & Modules
#include "utils/sensors.h"
#include "utils/others.h"
#include "data/remote_datasource.h"
#include "state/sensor/sensor_state.h"
#include "state/job/job_state.h"
#include "state/device/device_state.h"

// Globals
Sensor sensor;
RemoteDataSource remote;
OtherUtils utils;

// Ticker
Ticker ticker;
Ticker jobTicker;
Ticker deviceTicker;

// Setup
void setup()
{
    Serial.begin(115200);
    delay(1000); // Give serial time to initialize
    Serial.println("[DEBUG] Serial communication started");
    Serial.println("[DEBUG] Device ready to receive commands");
    Serial.flush();
    
    Wire.begin();
    Wire.setClock(400000);

    // // Initial update after Wi-Fi connected
    deviceState.updateFromSystem();

    // // Init sensors & modules
    // sensor.begin();
    // remote.begin();
    // jobState.begin();
    // jobState.startJob();

    // // Schedule job + sensor updater
    // jobTicker.attach(6, []() { jobState.tick(remote); });

    // // Schedule device info updater
    // ticker.attach(1, []() { utils.taskMaster(sensor.readTemperature(), sensor.readHeartBeat()); });

    deviceTicker.attach(1, []()
                        { deviceState.updateFromSystem(); });
}

// Main Loop
void loop()
{
    utils.onDeviceStateChange();

    // // Update sensor state
    // sensorState.setState(
    //     sensor.readTemperature(),
    //     sensor.readHeartBeat());

    // remote.loop(); // MQTT keep-alive
    
    // Small delay to prevent overwhelming the serial buffer
    delay(10);
}
