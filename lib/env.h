#define WIFI_SSID "placeholder"     // WiFi credentials
#define WIFI_PASSWORD "placeholder" // WiFi credentials

#define AZURE_IOT_HOST "Moorgan-IoT-Hub.azure-devices.net" // Azure IoT Hub hostname
#define AZURE_IOT_PORT 8883                                // Azure IoT Hub port for MQTT over TLS
#define AZURE_IOT_DEVICE_ID "1"                            // Device ID in Azure IoT Hub

// SAS Token akan digenerate dari SharedAccessKey, bukan ditulis langsung
#define AZURE_IOT_SAS_TOKEN "SharedAccessSignature sr=Moorgan-IoT-Hub.azure-devices.net/devices/1&sig=...&se=..."
#define AZURE_IOT_TOPIC "devices/1/messages/events/"
