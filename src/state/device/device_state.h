#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "../../../lib/env.h"

// Forward declaration
class OtherUtils;

// Callback type
typedef void (*StateCallback)();

class DeviceState
{
public:
    // Constant device information
    const String deviceName = DEVICE_NAME;
    const String deviceType = DEVICE_TYPE;
    String deviceId;
    const String firmwareVersion = FIRMWARE_VERSION;
    const String boardType = BOARD_TYPE;
    const String macAddressStatic = MAC_ADDRESS;
    const String installationDate = INSTALLATION_DATE;
    const String location = LOCATION;

private:
    // Dynamic status info
    String currentStatus = "Offline";
    String lastSeen = "Not available";
    String connectionType = "Wi-Fi";
    String ipAddress = "Not available";
    String macAddress = "Not available"; // live mac from WiFi.macAddress()
    String signalStrength = "Not available";

    String powerSource = "Battery";
    String batteryLevel = "Not available";
    String chargingStatus = "Not Charging";
    String voltageReading = "Not available";

    StateCallback onChange = nullptr;

    bool hasChanged(String oldVal, String newVal);
    bool applyChange(String &field, const String &newVal);

public:
    void updateFromSystem();
    void printState();
    void setListener(StateCallback callback);
    void handleSerialCommand(const String &command);
};

// Singleton instance
extern DeviceState deviceState;
