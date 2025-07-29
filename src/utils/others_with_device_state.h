#pragma once

#include "others.h"
#include "../state/device/device_state.h"

// Extended OtherUtils class that integrates with DeviceState
class OtherUtilsWithDeviceState : public OtherUtils
{
private:
    DeviceState* deviceStatePtr;

public:
    OtherUtilsWithDeviceState(DeviceState* deviceState = nullptr) 
        : deviceStatePtr(deviceState) {}
    
    void setDeviceState(DeviceState* deviceState) {
        deviceStatePtr = deviceState;
    }
    
    // Override the command handler to use actual DeviceState
    void onDeviceStateChange()
    {
        if (Serial.available())
        {
            String command = Serial.readStringUntil('\n');
            command.trim();

            if (command.length() > 0)
            {
                if (deviceStatePtr != nullptr) {
                    // Call the actual DeviceState handler
                    deviceStatePtr->handleSerialCommand(command);
                } else {
                    // Fallback to basic handler
                    handleDeviceCommand(command);
                }
            }
        }
    }
    
    // Enhanced command handling with device state integration
    void handleDeviceCommandWithState(const String& command)
    {
        if (deviceStatePtr == nullptr) {
            handleDeviceCommand(command); // Fallback to basic
            return;
        }
        
        // Extended commands that work with DeviceState
        if (command == "sleep") {
            Serial.println("Entering sleep mode...");
            // deviceStatePtr->enterSleepMode();
        } else if (command == "wake") {
            Serial.println("Waking up device...");
            // deviceStatePtr->wakeUp();
        } else if (command.startsWith("config ")) {
            String configData = command.substring(7);
            Serial.println("Updating config: " + configData);
            // deviceStatePtr->parseConfigJSON(configData);
        } else {
            // Use the DeviceState handler for other commands
            deviceStatePtr->handleSerialCommand(command);
        }
    }
};
