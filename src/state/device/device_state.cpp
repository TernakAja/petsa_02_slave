#include "device_state.h"
#include "../../utils/others.h"
#include <EEPROM.h>

// Define the global deviceState instance
DeviceState deviceState;

// Constructor implementation
DeviceState::DeviceState() {
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
        Serial.println("{");
        Serial.println("  \"device_info\": {");
        // Use runtime values if available, otherwise fall back to defaults
        String currentDeviceName = deviceNameRuntime.isEmpty() ? deviceName : deviceNameRuntime;
        String currentLocation = locationRuntime.isEmpty() ? location : locationRuntime;
        String currentInstallationDate = installationDateRuntime.isEmpty() ? installationDate : installationDateRuntime;
        
        Serial.println("    \"device_name\": \"" + currentDeviceName + "\",");
        Serial.println("    \"device_repo\": \"" + deviceRepo + "\",");
        Serial.println("    \"device_type\": \"" + deviceType + "\",");
        Serial.println("    \"device_id\": \"" + deviceId + "\",");
        Serial.println("    \"firmware_version\": \"" + firmwareVersion + "\",");
        Serial.println("    \"board_type\": \"" + boardType + "\",");
        Serial.println("    \"mac_address\": \"" + WiFi.macAddress() + "\",");
        Serial.println("    \"installation_date\": \"" + currentInstallationDate + "\",");
        Serial.println("    \"location\": \"" + currentLocation + "\",");
        Serial.println("    \"wifi_ssid\": \"" + wifiSSID + "\",");
        Serial.println("    \"wifi_password\": \"[HIDDEN]\"");
        Serial.println("  },");
        
        // Add connectivity and power status
        printState();
        
        Serial.flush(); // Ensure data is sent immediately
        // Serial.println("[DEBUG] Response sent and flushed"); // Removed to avoid JSON parsing issues
    }
    else if(command == "INFO_CONNECTION") {
        printState();
    }
    else if (command.startsWith("SET_WIFI:")) {
        handleWifiConfig(command);
    }
    else if (command.startsWith("SET_DEVICE:")) {
        handleDeviceConfig(command);
    }
    else if (command == "SAVE_CONFIG") {
        saveConfigToEEPROM();
    }
    else if (command == "RESET_CONFIG") {
        resetConfigToDefaults();
    }
    else {
        Serial.println("[CONFIG] ERROR: Unknown command '" + command + "'");
    }
}

void DeviceState::updateFromSystem()
{
    // Initialize deviceId if not set
    if (deviceId.isEmpty()) {
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
    if (voltage < 1.0) {
        powerSource = "USB (PC Connection)";
        chargingStatus = "External Power";
        batteryLevel = "N/A (USB Powered)";
    } else if (voltage >= 3.0 && voltage <= 4.2) {
        powerSource = "Battery";
        chargingStatus = "Not Charging";
        // Calculate battery percentage (rough estimation for Li-ion)
        float batteryPercent = ((voltage - 3.0) / (4.2 - 3.0)) * 100;
        batteryLevel = String((int)batteryPercent) + "%";
    } else if (voltage > 4.2) {
        powerSource = "External/Charging";
        chargingStatus = "Charging";
        batteryLevel = "100%+ (Charging)";
    } else if (voltage >= 1.0 && voltage < 3.0) {
        powerSource = "Battery (Low)";
        chargingStatus = "Not Charging";
        batteryLevel = "Low/Critical";
    } else {
        powerSource = "Unknown";
        chargingStatus = "Unknown";
        batteryLevel = "Unknown";
    }
}

void DeviceState::printState()
{
    Serial.println("  \"connectivity_status\": {");
    Serial.println("    \"current_status\": \"" + currentStatus + "\",");
    Serial.println("    \"last_seen\": \"" + lastSeen + "\",");
    Serial.println("    \"connection_type\": \"" + connectionType + "\",");
    Serial.println("    \"ip_address\": \"" + ipAddress + "\",");
    Serial.println("    \"signal_strength\": \"" + signalStrength + "\"");
    Serial.println("  },");

    // Update power status with real-time readings before printing
    updatePowerStatus();
    
    Serial.println("  \"power_status\": {");
    Serial.println("    \"power_source\": \"" + powerSource + "\",");
    Serial.println("    \"battery_level\": \"" + batteryLevel + "\",");
    Serial.println("    \"charging_status\": \"" + chargingStatus + "\",");
    Serial.println("    \"voltage_reading\": \"" + voltageReading + "\"");
    Serial.println("  }");

    Serial.println("}");
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
    
    if (firstColon == -1 || secondColon == -1) {
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
    
    if (firstColon == -1 || secondColon == -1) {
        Serial.println("[CONFIG] ERROR: Invalid device command format. Use: SET_DEVICE:field:value");
        return;
    }
    
    String field = command.substring(firstColon + 1, secondColon);
    String value = command.substring(secondColon + 1);
    
    // Update the appropriate field
    if (field == "DEVICE_NAME") {
        deviceNameRuntime = value;
        Serial.println("[CONFIG] SUCCESS: Device name updated to '" + value + "'");
    }
    else if (field == "LOCATION") {
        locationRuntime = value;
        Serial.println("[CONFIG] SUCCESS: Location updated to '" + value + "'");
    }
    else if (field == "INSTALLATION_DATE") {
        installationDateRuntime = value;
        Serial.println("[CONFIG] SUCCESS: Installation date updated to '" + value + "'");
    }
    else {
        Serial.println("[CONFIG] ERROR: Unknown field '" + field + "'. Supported: DEVICE_NAME, LOCATION, INSTALLATION_DATE");
        return;
    }
    
    Serial.println("[CONFIG] Note: Use SAVE_CONFIG to persist changes");
}

void DeviceState::saveConfigToEEPROM()
{
    Serial.println("[CONFIG] Saving configuration to EEPROM...");
    
    EEPROM.begin(512); // Initialize EEPROM with 512 bytes
    
    // Create JSON config string
    String config = "{";
    config += "\"wifi_ssid\":\"" + wifiSSID + "\",";
    config += "\"wifi_password\":\"" + wifiPassword + "\",";
    config += "\"device_name\":\"" + deviceNameRuntime + "\",";
    config += "\"location\":\"" + locationRuntime + "\",";
    config += "\"installation_date\":\"" + installationDateRuntime + "\"";
    config += "}";
    
    // Write config length first
    int configLen = config.length();
    EEPROM.write(0, configLen & 0xFF);
    EEPROM.write(1, (configLen >> 8) & 0xFF);
    
    // Write config data
    for (int i = 0; i < configLen && i < 510; i++) {
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
    
    if (configLen > 0 && configLen < 510) {
        // Read config data
        String config = "";
        for (int i = 0; i < configLen; i++) {
            config += char(EEPROM.read(i + 2));
        }
        
        Serial.println("[CONFIG] Loaded from EEPROM: " + config);
        
        // Parse JSON (simple parsing for key values)
        parseConfigJSON(config);
    } else {
        Serial.println("[CONFIG] No valid config found in EEPROM, using defaults");
        initializeDefaults();
    }
    
    EEPROM.end();
}

void DeviceState::parseConfigJSON(const String &json)
{
    // Simple JSON parsing for our specific format
    int pos = 0;
    
    // Extract wifi_ssid
    pos = json.indexOf("\"wifi_ssid\":\"");
    if (pos != -1) {
        pos += 13; // Skip key
        int endPos = json.indexOf("\"", pos);
        if (endPos != -1) {
            wifiSSID = json.substring(pos, endPos);
        }
    }
    
    // Extract wifi_password
    pos = json.indexOf("\"wifi_password\":\"");
    if (pos != -1) {
        pos += 17; // Skip key
        int endPos = json.indexOf("\"", pos);
        if (endPos != -1) {
            wifiPassword = json.substring(pos, endPos);
        }
    }
    
    // Extract device_name
    pos = json.indexOf("\"device_name\":\"");
    if (pos != -1) {
        pos += 15; // Skip key
        int endPos = json.indexOf("\"", pos);
        if (endPos != -1) {
            deviceNameRuntime = json.substring(pos, endPos);
        }
    }
    
    // Extract location
    pos = json.indexOf("\"location\":\"");
    if (pos != -1) {
        pos += 12; // Skip key
        int endPos = json.indexOf("\"", pos);
        if (endPos != -1) {
            locationRuntime = json.substring(pos, endPos);
        }
    }
    
    // Extract installation_date
    pos = json.indexOf("\"installation_date\":\"");
    if (pos != -1) {
        pos += 20; // Skip key
        int endPos = json.indexOf("\"", pos);
        if (endPos != -1) {
            installationDateRuntime = json.substring(pos, endPos);
        }
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
