#include "job_state.h"
#include "../sensor/sensor_state.h"
#include "../device/device_state.h"

// Forward declarations - both states are defined in their respective .cpp files
extern SensorState sensorState;
extern DeviceState deviceState;
JobState jobState(sensorState, deviceState);
