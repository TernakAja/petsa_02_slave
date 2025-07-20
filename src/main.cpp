#include <Wire.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>

// Utils & Modules
#include "utils/sensors.h"
#include "utils/others.h"
#include "data/remote_datasource.h"
#include "state/sensor/sensor_state.h"
#include "state/job/job_state.h"

// Globals
Sensor sensor;
Ticker ticker;
RemoteDataSource remote;
OtherUtils utils;
Ticker jobTicker;
JobState jobState;

// Setup
void setup()
{
    // Initialize Serial: setup serial communication
    Serial.begin(115200);
    Wire.begin();
    Wire.setClock(400000);

    // Initialize Connection & Sensors
    sensor.begin();
    remote.begin();

    // Job State: setup job state
    jobState.begin();
    jobState.startJob();

    // Set up secondary periodic tasks
    ticker.attach(6, []()
                  { jobState.tick(remote); });
    ticker.attach(1, []()
                  { utils.taskMaster(sensor.readTemperature(), sensor.readHeartBeat()); });
}

// Main Loop: non-delaying loop
void loop()
{
    // Set current sensor state : This should be called periodically to update the sensor state
    sensorState.setState(
        sensor.readTemperature(),
        sensor.readHeartBeat());

    remote.loop(); // Keep MQTT alive
}
