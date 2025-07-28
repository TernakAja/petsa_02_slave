#include "device_state.h"
#include "../../utils/others.h"
#include "../../data/remote_datasource.h"
#include <EEPROM.h>
#include <ArduinoJson.h>

// Define the global deviceState instance
DeviceState deviceState;

// Constructor implementation
DeviceState::DeviceState()
{
    // Initialize runtime variables with defaults
    initializeDefaults();

    // Try to load saved configuration from EEPROM
    loadConfigFromEEPROM();
}

bool DeviceState::hasChanged(String oldVal, String newVal)
{
    return oldVal != newVal;
}

bool DeviceState::applyChange(String &field, const String &newVal)
{
    if (hasChanged(field, newVal))
    {
        field = newVal;
        return true;
    }
    return false;
}

void DeviceState::handleSerialCommand(const String &command)
{
    Serial.print("[DEBUG] Processing command: '");
    Serial.print(command);
    Serial.println("'");

    if (command == "INFO")
    {
        Serial.println("[DEBUG] INFO command matched, sending response...");

        // Create JSON document
        JsonDocument doc;
        JsonObject deviceInfo = doc["device_info"].to<JsonObject>();

        // Use runtime values if available, otherwise fall back to defaults
        String currentDeviceName = deviceNameRuntime.isEmpty() ? deviceName : deviceNameRuntime;
        String currentLocation = locationRuntime.isEmpty() ? location : locationRuntime;
        String currentInstallationDate = installationDateRuntime.isEmpty() ? installationDate : installationDateRuntime;

        deviceInfo["device_name"] = currentDeviceName;
        deviceInfo["device_repo"] = deviceRepo;
        deviceInfo["device_type"] = deviceType;
        deviceInfo["device_id"] = deviceId;
        deviceInfo["firmware_version"] = firmwareVersion;
        deviceInfo["board_type"] = boardType;
        deviceInfo["mac_address"] = WiFi.macAddress();
        deviceInfo["installation_date"] = currentInstallationDate;
        deviceInfo["location"] = currentLocation;
        deviceInfo["wifi_ssid"] = wifiSSID;
        deviceInfo["wifi_password"] = "[HIDDEN]";

        // Add connectivity and power status
        addStateToJson(doc);

        // Print the JSON
        serializeJsonPretty(doc, Serial);
        Serial.println();

        Serial.flush(); // Ensure data is sent immediately
        // Serial.println("[DEBUG] Response sent and flushed"); // Removed to avoid JSON parsing issues
    }
    else if (command == "INFO_CONNECTION")
    {
        JsonDocument doc;
        addStateToJson(doc);
        serializeJsonPretty(doc, Serial);
        Serial.println();
    }
    else if (command.startsWith("SET_WIFI:"))
    {
        handleWifiConfig(command);
    }
    else if (command.startsWith("SET_DEVICE:"))
    {
        handleDeviceConfig(command);
    }
    else if (command == "SAVE_CONFIG")
    {
        saveConfigToEEPROM();
    }
    else if (command == "RESET_CONFIG")
    {
        resetConfigToDefaults();
    }
    else
    {
        Serial.println("[CONFIG] ERROR: Unknown command '" + command + "'");
    }
}

void DeviceState::updateFromSystem()
{
    // Initialize deviceId if not set
    if (deviceId.isEmpty())
    {
        deviceId = OtherUtils::getDeviceId();
    }

    bool changed = false;

    if (WiFi.isConnected())
    {
        changed |= applyChange(currentStatus, "Online");
        changed |= applyChange(ipAddress, WiFi.localIP().toString());
        changed |= applyChange(macAddress, WiFi.macAddress());
        changed |= applyChange(signalStrength, String(WiFi.RSSI()) + " dBm");
    }
    else
    {
        changed |= applyChange(currentStatus, "Offline");
        changed |= applyChange(ipAddress, "Not available");
        changed |= applyChange(signalStrength, "Not available");
    }

    // Update power status with real hardware readings
    updatePowerStatus();

    if (changed && onChange != nullptr)
    {
        onChange();
    }
}

void DeviceState::updatePowerStatus()
{
    // Read ADC value (0-1024 on ESP8266)
    int adcValue = analogRead(A0);

    // Convert ADC to voltage (ESP8266 ADC reference is typically 1.0V, but with voltage divider it can read up to 3.3V)
    // For battery monitoring, typical voltage divider gives us: Vout = Vin * (R2/(R1+R2))
    // Assuming a 3.3V max input with voltage divider
    float voltage = (adcValue / 1024.0) * 3.3;

    // Update voltage reading
    voltageReading = String(voltage, 2) + "V";

    // Determine power source based on voltage level
    // When connected to PC via USB, ESP8266 is powered externally
    // Low voltage (< 1.0V) typically indicates USB power with no battery connected
    // Higher voltage (> 2.5V) indicates battery power
    if (voltage < 1.0)
    {
        powerSource = "USB (PC Connection)";
        chargingStatus = "External Power";
        batteryLevel = "N/A (USB Powered)";
    }
    else if (voltage >= 3.0 && voltage <= 4.2)
    {
        powerSource = "Battery";
        chargingStatus = "Not Charging";
        // Calculate battery percentage (rough estimation for Li-ion)
        float batteryPercent = ((voltage - 3.0) / (4.2 - 3.0)) * 100;
        batteryLevel = String((int)batteryPercent) + "%";
    }
    else if (voltage > 4.2)
    {
        powerSource = "External/Charging";
        chargingStatus = "Charging";
        batteryLevel = "100%+ (Charging)";
    }
    else if (voltage >= 1.0 && voltage < 3.0)
    {
        powerSource = "Battery (Low)";
        chargingStatus = "Not Charging";
        batteryLevel = "Low/Critical";
    }
    else
    {
        powerSource = "Unknown";
        chargingStatus = "Unknown";
        batteryLevel = "Unknown";
    }
}

void DeviceState::addStateToJson(JsonDocument &doc)
{
    JsonObject connectivityStatus = doc["connectivity_status"].to<JsonObject>();
    connectivityStatus["current_status"] = currentStatus;
    connectivityStatus["last_seen"] = lastSeen;
    connectivityStatus["connection_type"] = connectionType;
    connectivityStatus["ip_address"] = ipAddress;
    connectivityStatus["signal_strength"] = signalStrength;

    // Update power status with real-time readings before adding to JSON
    updatePowerStatus();

    JsonObject powerStatus = doc["power_status"].to<JsonObject>();
    powerStatus["power_source"] = powerSource;
    powerStatus["battery_level"] = batteryLevel;
    powerStatus["charging_status"] = chargingStatus;
    powerStatus["voltage_reading"] = voltageReading;
}

void DeviceState::printState()
{
    JsonDocument doc;
    addStateToJson(doc);
    serializeJsonPretty(doc, Serial);
    Serial.println();
}

void DeviceState::setListener(StateCallback callback)
{
    onChange = callback;
}

void DeviceState::handleWifiConfig(const String &command)
{
    // Parse SET_WIFI:ssid:password
    int firstColon = command.indexOf(':', 8); // Skip "SET_WIFI"
    int secondColon = command.indexOf(':', firstColon + 1);

    if (firstColon == -1 || secondColon == -1)
    {
        Serial.println("[CONFIG] ERROR: Invalid WiFi command format. Use: SET_WIFI:ssid:password");
        return;
    }

    String ssid = command.substring(firstColon + 1, secondColon);
    String password = command.substring(secondColon + 1);

    // Store in runtime variables
    wifiSSID = ssid;
    wifiPassword = password;

    Serial.println("[CONFIG] SUCCESS: WiFi config updated - SSID: '" + ssid + "'");
    Serial.println("[CONFIG] Note: Use SAVE_CONFIG to persist changes");
}

void DeviceState::handleDeviceConfig(const String &command)
{
    // Parse SET_DEVICE:field:value
    int firstColon = command.indexOf(':', 10); // Skip "SET_DEVICE"
    int secondColon = command.indexOf(':', firstColon + 1);

    if (firstColon == -1 || secondColon == -1)
    {
        Serial.println("[CONFIG] ERROR: Invalid device command format. Use: SET_DEVICE:field:value");
        return;
    }

    String field = command.substring(firstColon + 1, secondColon);
    String value = command.substring(secondColon + 1);

    // Update the appropriate field
    if (field == "DEVICE_NAME")
    {
        deviceNameRuntime = value;
        Serial.println("[CONFIG] SUCCESS: Device name updated to '" + value + "'");
    }
    else if (field == "LOCATION")
    {
        locationRuntime = value;
        Serial.println("[CONFIG] SUCCESS: Location updated to '" + value + "'");
    }
    else if (field == "INSTALLATION_DATE")
    {
        installationDateRuntime = value;
        Serial.println("[CONFIG] SUCCESS: Installation date updated to '" + value + "'");
    }
    else
    {
        Serial.println("[CONFIG] ERROR: Unknown field '" + field + "'. Supported: DEVICE_NAME, LOCATION, INSTALLATION_DATE");
        return;
    }

    Serial.println("[CONFIG] Note: Use SAVE_CONFIG to persist changes");
}

void DeviceState::saveConfigToEEPROM()
{
    Serial.println("[CONFIG] Saving configuration to EEPROM...");

    EEPROM.begin(512); // Initialize EEPROM with 512 bytes

    // Create JSON config
    JsonDocument configDoc;
    configDoc["wifi_ssid"] = wifiSSID;
    configDoc["wifi_password"] = wifiPassword;
    configDoc["device_name"] = deviceNameRuntime;
    configDoc["location"] = locationRuntime;
    configDoc["installation_date"] = installationDateRuntime;

    // Serialize to string
    String config;
    serializeJson(configDoc, config);

    // Write config length first
    int configLen = config.length();
    EEPROM.write(0, configLen & 0xFF);
    EEPROM.write(1, (configLen >> 8) & 0xFF);

    // Write config data
    for (int i = 0; i < configLen && i < 510; i++)
    {
        EEPROM.write(i + 2, config[i]);
    }

    EEPROM.commit();
    EEPROM.end();

    Serial.println("[CONFIG] SUCCESS: Configuration saved to EEPROM");
    Serial.println("[CONFIG] Saved: " + config);
}

void DeviceState::loadConfigFromEEPROM()
{
    EEPROM.begin(512);

    // Read config length
    int configLen = EEPROM.read(0) | (EEPROM.read(1) << 8);

    if (configLen > 0 && configLen < 510)
    {
        // Read config data
        String config = "";
        for (int i = 0; i < configLen; i++)
        {
            config += char(EEPROM.read(i + 2));
        }

        Serial.println("[CONFIG] Loaded from EEPROM: " + config);

        // Parse JSON (simple parsing for key values)
        parseConfigJSON(config);
    }
    else
    {
        Serial.println("[CONFIG] No valid config found in EEPROM, using defaults");
        initializeDefaults();
    }

    EEPROM.end();
}

void DeviceState::parseConfigJSON(const String &json)
{
    JsonDocument configDoc;
    DeserializationError error = deserializeJson(configDoc, json);

    if (error)
    {
        Serial.println("[CONFIG] Error parsing JSON: " + String(error.c_str()));
        Serial.println("[CONFIG] Using defaults instead");
        initializeDefaults();
        return;
    }

    // Extract values using ArduinoJson
    if (configDoc.containsKey("wifi_ssid"))
    {
        wifiSSID = configDoc["wifi_ssid"].as<String>();
    }

    if (configDoc.containsKey("wifi_password"))
    {
        wifiPassword = configDoc["wifi_password"].as<String>();
    }

    if (configDoc.containsKey("device_name"))
    {
        deviceNameRuntime = configDoc["device_name"].as<String>();
    }

    if (configDoc.containsKey("location"))
    {
        locationRuntime = configDoc["location"].as<String>();
    }

    if (configDoc.containsKey("installation_date"))
    {
        installationDateRuntime = configDoc["installation_date"].as<String>();
    }
}

void DeviceState::initializeDefaults()
{
    wifiSSID = "placeholder";
    wifiPassword = "placeholder";
    deviceNameRuntime = deviceName;
    locationRuntime = location;
    installationDateRuntime = installationDate;
}

void DeviceState::resetConfigToDefaults()
{
    Serial.println("[CONFIG] Resetting configuration to defaults...");

    initializeDefaults();
    saveConfigToEEPROM();

    Serial.println("[CONFIG] SUCCESS: Configuration reset to defaults");
}

// Deep sleep management functions
void DeviceState::prepareForDeepSleep(RemoteDataSource &remote)
{
    Serial.println("[DEVICE] Preparing for deep sleep...");
    
    // Update device status
    currentStatus = "Entering Sleep";
    
    // Clean disconnect MQTT before deep sleep to prevent crashes
    Serial.println("[DEVICE] Disconnecting MQTT...");
    remote.disconnect();
    delay(100);
    
    // Disconnect WiFi properly
    Serial.println("[DEVICE] Disconnecting WiFi...");
    WiFi.disconnect(true);
    delay(100);
    
    // Additional cleanup time
    Serial.println("[DEVICE] Entering deep sleep in 3 seconds...");
    delay(3000); // Give time for everything to shut down cleanly
    
    // Now it's safe to enter deep sleep
    enterDeepSleep(10e6);
}

void DeviceState::enterDeepSleep(unsigned long sleepTimeUs)
{
    Serial.printf("[DEVICE] Entering deep sleep for %lu seconds...\n", sleepTimeUs / 1000000);
    
    // Update status before sleep
    currentStatus = "Deep Sleep";
    
    // Enter deep sleep
    ESP.deepSleep(sleepTimeUs);
}
