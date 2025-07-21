#include "others.h"
#include "../state/device/device_state.h"

void OtherUtils::onDeviceStateChange()
{
    if (Serial.available())
    {
        String command = Serial.readStringUntil('\n');
        command.trim(); // remove any trailing newline or space
        deviceState.handleSerialCommand(command);
    }
}