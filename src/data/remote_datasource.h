#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
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
    // OtherUtils::getDeviceId()
    const String deviceId = "1";
    const String shareKey = AZURE_SHARED_KEY;
    String sasToken = requestSasToken(host, shareKey, 3600, "http://192.168.1.3:5000/sas-token");

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
        if (sasToken.startsWith("Error"))
        {
            Serial.printf("\n Unable getting SAS token - %s ", sasToken);
            return false;
        }
        String clientId = deviceId; // Typically just DeviceId
        Serial.printf("Connecting to Azure IoT Hub as %s...\n", clientId.c_str());
        Serial.printf("\n SAS Token : %s", sasToken);
        // Azure requires this format for username:
        // "<host>/<deviceId>/?api-version=2021-04-12"
        String username = String(AZURE_IOT_HOST) + "/" + deviceId + "/?api-version=2021-04-12";
        mqttClient.setServer(AZURE_IOT_HOST, 8883);
        // SAS Token is the password for Azure MQTT
        bool connected = mqttClient.connect(
            clientId.c_str(), // clientId: your deviceId
            username.c_str(), // username: full Azure format
            sasToken.c_str()  // password: SAS token
        );

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
        JsonDocument doc;
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
    // TODO :
    // 1. Generate token SAS dari API Moorgan
    // 2. Konfigurasi ulang workflow buat send data
    // 3. Buat function untuk create new device di azure,
    //  > if theres no device in database, then create new device to azure
    //  > else

    String requestSasToken(const String &uri, const String &key, int expiry, const String &serverUrl)
    {

        HTTPClient http;
        http.begin(wifiClient, serverUrl); // Must be http://<ip>:<port>/sas-token
        http.addHeader("Content-Type", "application/json");

        // Create valid JSON
        String jsonBody = "{\"uri\":\"" + uri + "\",\"key\":\"" + key + "\",\"expiry\":" + String(expiry) + "}";

        int httpCode = http.POST(jsonBody);
        String response;

        if (httpCode > 0)
        {
            Serial.println("HTTP Code: " + String(httpCode));
            response = http.getString();
        }
        else
        {
            Serial.println("HTTP Error: " + String(httpCode));
            response = "Error: " + String(httpCode);
        }

        http.end();
        return response;
    }
};
