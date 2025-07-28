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

// External references
extern JobState jobState;

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
    delay(1000); // Wait for Wi-Fi connection
    remote.connect();
    jobState.begin();
    jobState.startJob();

    // Job to Send Data to Azure
    jobTicker.attach(1.2, []()
                    { 
                         // Only tick if job is active and not ready for sleep
                        if (!jobState.isReadyForSleep()) {
                            jobState.tick(remote); 
                        }
                    });
}

// Main Loop
void loop()
{
    // Check if job is ready for deep sleep
    if (jobState.isReadyForSleep()) {
        // Stop all tickers before deep sleep
        Serial.println("Stopping tickers...");
        jobTicker.detach();
        ticker.detach();
        delay(100);
        
        // Prepare for deep sleep
        jobState.prepareForDeepSleep(remote);
        // This line should never be reached as ESP.deepSleep() resets the device
    }
    
    // Handle MQTT connection and messages
    remote.loop();
    
    // Detects Command from Serial
    utils.onDeviceStateChange();

    // Update sensor state
    sensorState.setState(
        sensor.readTemperature(),
        sensor.readHeartBeat());

    delay(10); // delay biar ga bentrokan
}
