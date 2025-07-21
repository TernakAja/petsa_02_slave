#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "../../../lib/env.h"

// Callback type
typedef void (*StateCallback)();

class DeviceState
{
public:
    // Constant device information
    const String deviceName = DEVICE_NAME;
    const String deviceType = DEVICE_TYPE;
    const String deviceId = DEVICE_ID;
    const String firmwareVersion = FIRMWARE_VERSION;
    const String boardType = BOARD_TYPE;
    const String macAddress = MAC_ADDRESS;
    const String installationDate = INSTALLATION_DATE;
    const String location = LOCATION;

private:
    // Dynamic status info
    String currentStatus = "Offline";
    String lastSeen = "Not available";
    String connectionType = "Wi-Fi";
    String ipAddress = "Not available";
    String signalStrength = "Not available";

    String powerSource = "Battery";
    String batteryLevel = "Not available";
    String chargingStatus = "Not Charging";
    String voltageReading = "Not available";

    StateCallback onChange = nullptr;

public:
    void updateFromSystem();
    void printState();
    void setListener(StateCallback callback);
};

// Singleton instance
extern DeviceState deviceState;
