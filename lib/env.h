#include "../src/utils/others.h"
#define WIFI_SSID "placeholder"     // WiFi credentials
#define WIFI_PASSWORD "placeholder" // WiFi credentials

#define AZURE_IOT_HOST "placeholder"                  // Azure IoT Hub hostname
#define AZURE_IOT_PORT 8883                           // Azure IoT Hub port for MQTT over TLS
#define AZURE_IOT_DEVICE_ID OtherUtils::getDeviceId() // Device ID in Azure IoT Hub

// SAS Token akan digenerate dari SharedAccessKey, bukan ditulis langsung
#define AZURE_IOT_SAS_TOKEN "placeholder"
#define AZURE_IOT_TOPIC "placeholder"
