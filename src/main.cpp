#include <Wire.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>

// Utils & Modules
#include "utils/sensors.h"
#include "state/device/device_state.h"
#include "utils/others.h"
#include "data/remote_datasource.h"
#include "state/sensor/sensor_state.h"
#include "state/job/job_state.h"

// Globals
Sensor sensor;
RemoteDataSource remote;
OtherUtils utils;

// Ticker
Ticker ticker;
Ticker jobTicker;

// Setup
void setup()
{
    utils.serialTimeInitialization();

    // Initial update after Wi-Fi connected
    deviceState.updateFromSystem();

    // Init sensors & modules
    sensor.begin();
    remote.begin();
    jobState.begin();
    jobState.startJob();

    // Debug Temprature
    ticker.attach(6, []()
                  { utils.taskMaster(sensorState.getTemperature(), sensorState.getBPM()); });

    // Job to Send Data to Azure
    jobTicker.attach(1.2, []()
                     { jobState.tick(remote); });
}

// Main Loop
void loop()
{
    // Detects Command from Serial
    utils.onDeviceStateChange();

    // Update sensor state
    sensorState.setState(
        sensor.readTemperature(),
        sensor.readHeartBeat());

    delay(10); // delay biar ga bentrokan
}
