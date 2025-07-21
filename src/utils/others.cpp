#include "others.h"
#include "../state/device/device_state.h"

void OtherUtils::onDeviceStateChange()
{
    if (Serial.available())
    {
        String command = Serial.readStringUntil('\n');
        command.trim(); // remove any trailing newline or space
        
        // Debug: Print received command
        Serial.print("[DEBUG] Received command: '");
        Serial.print(command);
        Serial.println("'");
        
        if (command.length() > 0) {
            deviceState.handleSerialCommand(command);
        }
    }
}