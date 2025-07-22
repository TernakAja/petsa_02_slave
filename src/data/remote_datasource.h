#pragma once

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ctime>
#include <vector>
#include "../../lib/env.h"
#include "../utils/others.h"

struct SensorData
{
    float temperature;
    float bpm;
    float spO2;
};

class RemoteDataSource
{
private:
    WiFiClientSecure wifiClient;
    PubSubClient mqttClient;

    const char *host = AZURE_IOT_HOST;
    const String deviceId = OtherUtils::getDeviceId();
    const char *sasToken = AZURE_IOT_SAS_TOKEN;

    unsigned long lastReadTime = 0;
    unsigned long lastSendTime = 0;
    std::vector<SensorData> dataBuffer;

public:
    RemoteDataSource() : mqttClient(wifiClient) {}

    void begin()
    {
        wifiClient.setInsecure(); // Accept all certificates (not safe for production)
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.print("Connecting to WiFi");
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

    void sendData(float pulseRate, float temperature, float spO2)
    {
        if (!mqttClient.connected())
        {
            Serial.println("MQTT not connected. Skipping send.");
            return;
        }
        StaticJsonDocument<256> doc;
        doc["deviceId"] = deviceId;
        doc["pulseRate"] = pulseRate;
        doc["temperature"] = temperature;
        doc["sp02"] = spO2;
        time_t now = time(nullptr);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
        doc["timestamp"] = timestamp;
        char payload[256];
        serializeJson(doc, payload);
        String topic = "devices/" + deviceId + "/messages/events/";
        if (mqttClient.publish(topic.c_str(), payload))
        {
            Serial.println("Payload sent:");
            Serial.println(payload);
        }
        else
        {
            Serial.println("Failed to publish message.");
        }
    }

private:
};
