#pragma once

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

class RemoteDataSource
{
private:
    WiFiClientSecure wifiClient;
    PubSubClient mqttClient;

    const char *host = AZURE_IOT_HOST;
    const int port = AZURE_IOT_PORT;
    const char *deviceId = AZURE_IOT_DEVICE_ID;
    const char *sasToken = AZURE_IOT_SAS_TOKEN;
    const char *topic = AZURE_IOT_TOPIC;

public:
    RemoteDataSource() : mqttClient(wifiClient) {}

    void begin()
    {
        wifiClient.setInsecure(); // Skip TLS cert validation (not safe for prod)
        mqttClient.setServer(host, port);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nWiFi connected.");
    }

    bool connect()
    {
        if (mqttClient.connected())
            return true;

        String clientId = deviceId;
        Serial.printf("Connecting to Azure IoT Hub as %s...\n", clientId.c_str());

        bool connected = mqttClient.connect(
            clientId.c_str(),
            String(String(AZURE_IOT_HOST) + "/" + deviceId).c_str(),
            sasToken);

        if (connected)
        {
            Serial.println("Connected to Azure IoT Hub.");
        }
        else
        {
            Serial.print("Connection failed. State: ");
            Serial.println(mqttClient.state());
        }

        return connected;
    }

    bool sendSensorData(float temperature, float bpm)
    {
        if (!mqttClient.connected())
        {
            if (!connect())
                return false;
        }

        String payload = "{\"temperature\":" + String(temperature, 2) + ",\"bpm\":" + String(bpm, 2) + "}";
        bool published = mqttClient.publish(topic, payload.c_str());

        if (published)
        {
            Serial.println("Data sent to Azure IoT Hub: " + payload);
        }
        else
        {
            Serial.println("Failed to send data to Azure.");
        }

        return published;
    }

    void loop()
    {
        mqttClient.loop();
    }
};
