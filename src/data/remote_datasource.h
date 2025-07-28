#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ctime>
#include <vector>
#include <time.h>

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
    const String deviceId = "1";
    const String shareKey = AZURE_SHARED_KEY;
    String sasToken = "";

public:
    RemoteDataSource() : mqttClient(wifiClient)
    {
        mqttClient.setCallback(mqttCallback);
        mqttClient.setBufferSize(512); // Increase buffer size
    }

    void begin()
    {
        wifiClient.setInsecure();
        wifiClient.setTimeout(10000); // 10 second timeout
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.print("Connecting to WiFi");
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nWiFi connected.");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

        // Generate fresh SAS token
        sasToken = requestSasToken(host, deviceId, shareKey);        
    }

    bool connect(int maxRetries = 3)
    {
        if (mqttClient.connected())
            return true;

        if (sasToken.isEmpty() || sasToken.startsWith("Error"))
        {
            Serial.printf("Invalid SAS token: %s\n", sasToken.c_str());
            // Try to regenerate token
            sasToken = requestSasToken(host, deviceId, shareKey);
            if (sasToken.isEmpty() || sasToken.startsWith("Error"))
            {
                return false;
            }
        }

        String clientId = deviceId;
        String username = String(AZURE_IOT_HOST) + "/" + deviceId + "/?api-version=2021-04-12";

        mqttClient.setServer(AZURE_IOT_HOST, 8883);
        mqttClient.setKeepAlive(60);
        mqttClient.setSocketTimeout(15); // 15 second socket timeout

        for (int attempt = 1; attempt <= maxRetries; attempt++)
        {
            Serial.printf("MQTT connection attempt %d/%d\n", attempt, maxRetries);
                        
            
            bool connected = mqttClient.connect(
                clientId.c_str(),
                username.c_str(),
                sasToken.c_str());

            if (connected)
            {
                Serial.println("Connected to Azure IoT Hub.");
                return true;
            }
            else
            {
                int state = mqttClient.state();
                Serial.printf("Connection failed. State: %d\n", state);
                
                // Print state meanings for debugging
                switch(state) {
                    case -4: Serial.println("MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time"); break;
                    case -3: Serial.println("MQTT_CONNECTION_LOST - the network connection was broken"); break;
                    case -2: Serial.println("MQTT_CONNECT_FAILED - the network connection failed"); break;
                    case -1: Serial.println("MQTT_DISCONNECTED - the client is disconnected cleanly"); break;
                    case 1: Serial.println("MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT"); break;
                    case 2: Serial.println("MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier"); break;
                    case 3: Serial.println("MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection"); break;
                    case 4: Serial.println("MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected"); break;
                    case 5: Serial.println("MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect"); break;
                    default: Serial.println("Unknown error"); break;
                }

                if (attempt < maxRetries)
                {
                    Serial.printf("Retrying in 3 seconds...\n");
                    delay(3000);
                }
            }
        }

        return false;
    }

    // Add this method to be called in main loop
    void loop()
    {
        if (mqttClient.connected())
        {
            mqttClient.loop();
        }
        else
        {
            // Try to reconnect if disconnected
            static unsigned long lastReconnectAttempt = 0;
            unsigned long now = millis();
            if (now - lastReconnectAttempt > 5000) // Try reconnect every 5 seconds
            {
                lastReconnectAttempt = now;
                Serial.println("MQTT disconnected. Attempting to reconnect...");
                connect(1); // Single retry attempt in loop
            }
        }
    }

    // Add method to check connection status
    bool isConnected()
    {
        return mqttClient.connected();
    }

    // Add disconnect method for clean shutdown
    void disconnect()
    {
        if (mqttClient.connected())
        {
            mqttClient.disconnect();
            Serial.println("MQTT disconnected.");
        }
    }

    void sendData(float pulseRate, float temperature, float spO2)
    {
        if (!mqttClient.connected())
        {
            Serial.println("MQTT not connected. Attempting reconnect...");
            if (!connect()) return;
        }

        // Use smaller JSON document to save memory
        JsonDocument doc;
        doc["deviceId"] = deviceId;
        doc["pulseRate"] = pulseRate;
        doc["temperature"] = temperature;
        doc["sp02"] = spO2;

        time_t now = time(nullptr);
        if (now > 8 * 3600 * 2) // Check if we have a reasonable time (after year 1970)
        {
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
            doc["timestamp"] = timestamp;
        }
        else
        {
            // Fallback: use millis-based timestamp if NTP failed
            unsigned long currentMillis = millis();
            char timestamp[32];
            snprintf(timestamp, sizeof(timestamp), "device-uptime-%lu", currentMillis);
            doc["timestamp"] = timestamp;
            Serial.println("Warning: Using device uptime for timestamp (NTP failed)");
        }

        char payload[200];
        size_t len = serializeJson(doc, payload);

        String topic = "devices/" + deviceId + "/messages/events/";

        if (mqttClient.publish(topic.c_str(), payload, len))
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

    String requestSasToken(const String &uri, const String &deviceId, const String &primaryKey)
    {        
        HTTPClient http;
        String serverUrl = "https://ternak-aja-backend-c7fad0cgb8dmchh0.canadacentral-01.azurewebsites.net/sas";
        http.begin(wifiClient, serverUrl);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(15000); // 15 second timeout

        // Create properly formatted JSON - ensure hostname matches AZURE_IOT_HOST
        String jsonBody = "{\"hostname\":\"" + String(AZURE_IOT_HOST) + "\",\"deviceId\":\"" + deviceId + "\",\"primaryKey\":\"" + primaryKey + "\"}";
        Serial.println("Request body: " + jsonBody);
        int httpCode = http.POST(jsonBody);
        String response;
        String token = "";

        if (httpCode == 200)
        {
            Serial.println("Token Status Code: " + String(httpCode));
            response = http.getString();            

            // Parse the JSON response
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, response);

            if (error)
            {
                Serial.print("JSON parsing failed: ");
                Serial.println(error.c_str());
                token = "Error: JSON parse failed";
            }
            else
            {
                // Extract the SAS token from response
                if (doc["data"]["sasToken"])
                {
                    token = doc["data"]["sasToken"].as<String>();
                    Serial.println("SAS Token: " + token);
                }
                else
                {
                    Serial.println("sasToken not found in response");
                    token = "Error: Token not found";
                }
            }
        }
        else
        {
            Serial.println("HTTP Error: " + String(httpCode));
            response = http.getString();
            Serial.println("Error response: " + response);
            token = "Error: " + String(httpCode);
        }

        http.end();
        return token;
    }
    // Add callback function
    static void mqttCallback(char *topic, byte *payload, unsigned int length)
    {
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");
        for (unsigned int i = 0; i < length; i++)
        {
            Serial.print((char)payload[i]);
        }
        Serial.println();
    }
};
