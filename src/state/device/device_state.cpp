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
        Serial.println("    \"device_type\": \"" + deviceType + "\",");
        Serial.println("    \"device_id\": \"" + deviceId + "\",");
        Serial.println("    \"firmware_version\": \"" + firmwareVersion + "\",");
        Serial.println("    \"board_type\": \"" + boardType + "\",");
        Serial.println("    \"mac_address\": \"" + macAddress + "\",");
        Serial.println("    \"installation_date\": \"" + installationDate + "\",");
        Serial.println("    \"location\": \"" + location + "\"");
        Serial.println("  }");
        Serial.println("}");
        Serial.flush(); // Ensure data is sent immediately
        Serial.println("[DEBUG] Response sent and flushed");
    }
    else
    {
        Serial.println("[DEBUG] Unknown command: '" + command + "'");
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

    if (changed && onChange != nullptr)
    {
        onChange();
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
