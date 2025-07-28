#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "../../../lib/env.h"

// Forward declarations
class OtherUtils;
class RemoteDataSource;

// Callback type
typedef void (*StateCallback)();

class DeviceState
{
public:
    DeviceState(); // Constructor
    // Constant device information
    const String deviceName = DEVICE_NAME;
    const String deviceType = DEVICE_TYPE;
    const String deviceRepo = DEVICE_REPO;
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
    String macAddress = "Not available";
    String signalStrength = "Not available";

    String powerSource = "Battery";
    String batteryLevel = "Not available";
    String chargingStatus = "Not Charging";
    String voltageReading = "Not available";

    // Runtime configuration variables
    String wifiSSID = "placeholder";
    String wifiPassword = "placeholder";
    String deviceNameRuntime;
    String locationRuntime;
    String installationDateRuntime;

    StateCallback onChange = nullptr;

    bool hasChanged(String oldVal, String newVal);
    bool applyChange(String &field, const String &newVal);

public:
    void updateFromSystem();
    void updatePowerStatus();
    void printState();
    void addStateToJson(JsonDocument &doc);
    void setListener(StateCallback callback);
    void handleSerialCommand(const String &command);

    // Configuration management
    void handleWifiConfig(const String &command);
    void handleDeviceConfig(const String &command);
    void saveConfigToEEPROM();
    void loadConfigFromEEPROM();
    void resetConfigToDefaults();

    // Deep sleep management
    void prepareForDeepSleep(RemoteDataSource &remote);
    void enterDeepSleep(unsigned long sleepTimeUs = 300e6); // Default 5 minutes

private:
    void parseConfigJSON(const String &json);
    void initializeDefaults();
};

// Singleton instance
extern DeviceState deviceState;
