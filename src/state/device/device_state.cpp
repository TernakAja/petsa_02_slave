#include "device_state.h"
#include "../../utils/others.h"

// Define the global deviceState instance
DeviceState deviceState;

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
        Serial.println("    \"device_name\": \"" + deviceName + "\",");
        Serial.println("    \"device_repo\": \"" + deviceRepo + "\",");
        Serial.println("    \"device_type\": \"" + deviceType + "\",");
        Serial.println("    \"device_id\": \"" + deviceId + "\",");
        Serial.println("    \"firmware_version\": \"" + firmwareVersion + "\",");
        Serial.println("    \"board_type\": \"" + boardType + "\",");
        Serial.println("    \"mac_address\": \"" + WiFi.macAddress() + "\",");
        Serial.println("    \"installation_date\": \"" + installationDate + "\",");
        Serial.println("    \"location\": \"" + location + "\"");
        Serial.println("  },");
        
        // Add connectivity and power status
        printState();
        
        Serial.flush(); // Ensure data is sent immediately
        // Serial.println("[DEBUG] Response sent and flushed"); // Removed to avoid JSON parsing issues
    }
    if(command == "INFO_CONNECTION") {
        printState();
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
