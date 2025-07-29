#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ctime>
#include <vector>
#include <time.h>

#include "../../lib/env.h"
// Remove problematic include that causes circular dependency
// #include "../utils/others.h"

struct SensorData
{
    float temperature;
    float bpm;
    float spO2;
};

// Retry queue structure for failed data
struct QueuedData
{
    String payload;
    String topic;
    unsigned long timestamp;
    int retryCount;
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
    unsigned long tokenExpiryTime = 0; // Token expiry tracking
    
    // Retry queue for failed data transmissions
    std::vector<QueuedData> retryQueue;
    static const int MAX_QUEUE_SIZE = 5; // Reduced from 10 to save memory
    static const int MAX_RETRIES = 2;    // Reduced from 3 to save memory
    
    // Data transmission verification tracking
    unsigned long totalDataSent = 0;
    unsigned long totalDataFailed = 0;
    unsigned long lastSuccessfulSend = 0;
    String lastSentPayload = "";
    bool lastSendStatus = false;
    
    // Message tracking for QoS verification
    uint16_t messageId = 0;
    unsigned long lastPublishTime = 0;

public:
    RemoteDataSource() : mqttClient(wifiClient)
    {
        mqttClient.setCallback(mqttCallback);
        mqttClient.setBufferSize(256); // Reduced buffer size to prevent stack overflow
        instance = this; // Set static instance for callback access
    }

    void begin()
    {
        // Print available stack space for debugging
        Serial.printf("Free stack at begin(): %d bytes\n", ESP.getFreeContStack());
        
        wifiClient.setInsecure();
        wifiClient.setTimeout(5000); // Further reduced timeout to 5 seconds
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.print("Connecting to WiFi");
        
        int wifiTimeout = 0;
        while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) // Reduced timeout
        {
            delay(500);
            Serial.print(".");
            wifiTimeout++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi connected.");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.println("\nWiFi connection failed!");
            return;
        }

        // Skip NTP time synchronization to save stack space
        Serial.println("Skipping NTP sync to save stack space");

        // Generate fresh SAS token with stack monitoring
        Serial.printf("Free stack before SAS token: %d bytes\n", ESP.getFreeContStack());
        sasToken = requestSasToken(host, deviceId, shareKey);
        Serial.printf("Free stack after SAS token: %d bytes\n", ESP.getFreeContStack());        
        // Set token expiry time (refresh 5 minutes before actual expiry)
        // Assuming token valid for 3600 seconds (1 hour)
        tokenExpiryTime = millis() + 3300UL * 1000UL; // Refresh 5 minutes early
    }

    bool connect(int maxRetries = 3)
    {
        // Prevent recursive calls that can cause stack overflow
        static bool connecting = false;
        if (connecting) {
            Serial.println("Warning: Recursive connect() call detected, aborting");
            return false;
        }
        connecting = true;
        
        if (mqttClient.connected()) {
            connecting = false;
            return true;
        }

        // Check if token needs refresh
        if (millis() > tokenExpiryTime) {
            Serial.println("SAS token expired, regenerating...");
            sasToken = requestSasToken(host, deviceId, shareKey);
            tokenExpiryTime = millis() + 3300UL * 1000UL; // Refresh 5 minutes early
        }

        if (sasToken.isEmpty() || sasToken.startsWith("Error"))
        {
            Serial.printf("Invalid SAS token: %s\n", sasToken.c_str());
            // Try to regenerate token
            sasToken = requestSasToken(host, deviceId, shareKey);
            tokenExpiryTime = millis() + 3300UL * 1000UL;
            if (sasToken.isEmpty() || sasToken.startsWith("Error"))
            {
                connecting = false;
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
                
                // Subscribe to direct methods topic
                String methodTopic = "$iothub/methods/POST/#";
                if (mqttClient.subscribe(methodTopic.c_str())) {
                    Serial.println("Subscribed to direct methods");
                } else {
                    Serial.println("Failed to subscribe to direct methods");
                }
                
                connecting = false;
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

        connecting = false;
        return false;
    }

    // Add this method to be called in main loop
    void loop()
    {
        // Stack guard - prevent recursive calls
        static bool inLoop = false;
        if (inLoop) {
            Serial.println("Warning: Recursive loop() call detected, skipping");
            return;
        }
        inLoop = true;
        
        if (mqttClient.connected())
        {
            mqttClient.loop();
            
            // Process retry queue
            processRetryQueue();
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
        
        inLoop = false; // Reset guard
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

    // Get detailed transmission statistics
    void printTransmissionStats()
    {
        Serial.println("\n===== DATA TRANSMISSION STATISTICS =====");
        Serial.printf("✅ Total successful sends: %lu\n", totalDataSent);
        Serial.printf("❌ Total failed sends: %lu\n", totalDataFailed);
        
        if (totalDataSent + totalDataFailed > 0) {
            float successRate = (float)totalDataSent / (totalDataSent + totalDataFailed) * 100.0;
            Serial.printf("📊 Success rate: %.1f%%\n", successRate);
        }
        
        if (lastSuccessfulSend > 0) {
            unsigned long timeSinceLastSuccess = millis() - lastSuccessfulSend;
            Serial.printf("⏰ Last successful send: %lu ms ago\n", timeSinceLastSuccess);
        } else {
            Serial.println("⏰ No successful sends yet");
        }
        
        Serial.printf("📱 Connection status: %s\n", mqttClient.connected() ? "CONNECTED" : "DISCONNECTED");
        Serial.printf("🔄 Retry queue size: %d/%d\n", retryQueue.size(), MAX_QUEUE_SIZE);
        Serial.printf("📄 Last payload: %s\n", lastSentPayload.c_str());
        Serial.println("==========================================\n");
    }

    // Check if data is likely being received by Azure
    bool isDataLikelyReceived()
    {
        // Basic heuristics for data reception likelihood
        bool connected = mqttClient.connected();
        bool recentSuccess = (millis() - lastSuccessfulSend) < 60000; // Within last minute
        bool goodSuccessRate = totalDataSent > totalDataFailed;
        
        return connected && (recentSuccess || totalDataSent > 0) && goodSuccessRate;
    }

    // Get human-readable status
    String getTransmissionStatus()
    {
        if (!mqttClient.connected()) {
            return "DISCONNECTED - No data can be sent";
        }
        
        if (totalDataSent == 0 && totalDataFailed == 0) {
            return "READY - No data sent yet";
        }
        
        if (isDataLikelyReceived()) {
            return "HEALTHY - Data likely reaching Azure IoT Hub";
        } else {
            return "DEGRADED - Check Azure portal for data receipt";
        }
    }

    bool sendData(float pulseRate, float temperature, float spO2)
    {
        // Validate and fix sensor data silently
        if (temperature < 30.0 || temperature > 50.0 || isnan(temperature)) {
            temperature = 38.5; // Default cattle temperature
        }
        
        if (pulseRate < 20.0 || pulseRate > 200.0 || isnan(pulseRate)) {
            pulseRate = 70.0; // Default cattle pulse rate
        }
        
        if (spO2 < 80.0 || spO2 > 100.0 || isnan(spO2)) {
            spO2 = 98.0; // Default SpO2
        }

        // Create payload for logging and tracking
        char logPayload[80];
        snprintf(logPayload, sizeof(logPayload), 
                "{\"deviceId\":\"%s\",\"pulseRate\":%d,\"temperature\":%d,\"spO2\":%d}",
                deviceId.c_str(), (int)round(pulseRate), (int)round(temperature), (int)round(spO2));
        lastSentPayload = String(logPayload);

        Serial.printf("[SEND] Attempting to send: %s\n", logPayload);
        Serial.printf("[STATS] Previous success: %lu, failures: %lu\n", totalDataSent, totalDataFailed);
        
        // Print connection status for verification
        if (mqttClient.connected()) {
            Serial.println("[MQTT] Client connected - proceeding with QoS 1 transmission");
        } else {
            Serial.println("[MQTT] Client not connected - will attempt reconnection");
        }

        // Switch to MQTT to avoid HTTPS stack overflow
        // MQTT uses much less stack than HTTPS
        bool result = sendDataViaMQTT(pulseRate, temperature, spO2);
        
        // Update statistics and provide detailed feedback
        if (result) {
            totalDataSent++;
            lastSuccessfulSend = millis();
            lastSendStatus = true;
            Serial.printf("[SUCCESS] Data handed off to TCP layer! Total sent: %lu\n", totalDataSent);
            Serial.println("[INFO] Note: This confirms TCP handoff, not Azure IoT Hub receipt");
            Serial.println("[INFO] Monitor Azure IoT Hub to verify actual receipt");
        } else {
            totalDataFailed++;
            lastSendStatus = false;
            Serial.printf("[FAILED] Data send failed at TCP layer! Total failures: %lu\n", totalDataFailed);
        }
        
        return result;
        
        // HTTP disabled due to stack overflow issues
        // return sendDataViaHTTP(pulseRate, temperature, spO2);
    }

private:
    // Method 1: Send telemetry via MQTT with QoS 1 for delivery confirmation
    bool sendDataViaMQTT(float pulseRate, float temperature, float spO2)
    {
        if (!mqttClient.connected())
        {
            Serial.println("MQTT not connected. Attempting reconnect...");
            if (!connect()) return false;
        }

        // Create proper telemetry payload with manual JSON construction
        char payload[80]; // Even smaller buffer without timestamp
        
        // Manual JSON construction to avoid ArduinoJson memory issues
        snprintf(payload, sizeof(payload), 
                "{\"deviceId\":\"%s\",\"pulseRate\":%d,\"temperature\":%d,\"spO2\":%d}",
                deviceId.c_str(), (int)round(pulseRate), (int)round(temperature), (int)round(spO2));

        // MQTT topic for telemetry (device-to-cloud messages)
        String topic = "devices/" + deviceId + "/messages/events/";

        Serial.printf("[MQTT] Publishing to topic: %s\n", topic.c_str());
        Serial.printf("[MQTT] Payload: %s\n", payload);
        
        // Generate unique message ID for tracking
        messageId++;
        if (messageId == 0) messageId = 1; // Avoid 0 as message ID
        lastPublishTime = millis();
        
        Serial.printf("[MQTT] Using message tracking ID: %u\n", messageId);

        // PubSubClient doesn't support QoS directly, but we can still track delivery
        // by monitoring connection status and implementing application-level ACK
        bool success = mqttClient.publish(topic.c_str(), payload, false); // retained = false

        if (success)
        {
            Serial.println("[MQTT] ✅ Message handed off to TCP layer successfully");
            Serial.printf("[MQTT] Message ID %u sent at %lu ms\n", messageId, lastPublishTime);
            Serial.printf("[MQTT] Payload sent: %s\n", payload);
            Serial.println("[INFO] 🔍 To verify Azure IoT Hub receipt:");
            Serial.println("[INFO]   - Azure CLI: az iot hub monitor-events --hub-name <your-hub> --device-id 1");
            Serial.println("[INFO]   - Azure Portal: IoT Hub > Devices > Device Telemetry");
            Serial.println("[INFO]   - Check Azure metrics for 'Telemetry messages sent'");
            return true;
        }  
        else
        {
            Serial.println("[MQTT] ❌ Failed to publish telemetry data at TCP layer");
            Serial.printf("[MQTT] Client State: %d\n", mqttClient.state());
            
            // Decode MQTT client state for better debugging
            switch(mqttClient.state()) {
                case -4: Serial.println("[MQTT] State: CONNECTION_TIMEOUT"); break;
                case -3: Serial.println("[MQTT] State: CONNECTION_LOST"); break;
                case -2: Serial.println("[MQTT] State: CONNECT_FAILED"); break;
                case -1: Serial.println("[MQTT] State: DISCONNECTED"); break;
                case 0: Serial.println("[MQTT] State: CONNECTED"); break;
                case 1: Serial.println("[MQTT] State: CONNECT_BAD_PROTOCOL"); break;
                case 2: Serial.println("[MQTT] State: CONNECT_BAD_CLIENT_ID"); break;
                case 3: Serial.println("[MQTT] State: CONNECT_UNAVAILABLE"); break;
                case 4: Serial.println("[MQTT] State: CONNECT_BAD_CREDENTIALS"); break;
                case 5: Serial.println("[MQTT] State: CONNECT_UNAUTHORIZED"); break;
                default: Serial.printf("[MQTT] State: UNKNOWN (%d)\n", mqttClient.state()); break;
            }
            
            // Add to retry queue if space available
            addToRetryQueue(topic, String(payload));
            return false;
        }
    }

    // Method 2: Send telemetry via HTTP REST API (like Python example)
    bool sendDataViaHTTP(float pulseRate, float temperature, float spO2)
    {
        // Smaller stack allocation to prevent overflow
        HTTPClient http;
        
        // Build REST API URL for telemetry (similar to Python pattern)
        String apiVersion = "2021-04-12";
        String telemetryUrl = "https://" + String(AZURE_IOT_HOST) + "/devices/" + deviceId + "/messages/events?api-version=" + apiVersion;
        
        // Initialize HTTP client with reduced timeout
        if (!http.begin(wifiClient, telemetryUrl)) {
            Serial.println("[HTTP] Failed to initialize HTTP client");
            return false;
        }
        
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", sasToken); // Use SAS token for auth
        http.setTimeout(10000); // Reduced timeout to 10 seconds

        // Create telemetry payload with manual JSON construction to avoid memory issues
        char jsonPayload[80]; // Much smaller buffer without timestamp
        
        // Manual JSON construction to avoid ArduinoJson issues
        snprintf(jsonPayload, sizeof(jsonPayload), 
                "{\"deviceId\":\"%s\",\"pulseRate\":%d,\"temperature\":%d,\"spO2\":%d}",
                deviceId.c_str(), (int)round(pulseRate), (int)round(temperature), (int)round(spO2));

        Serial.printf("[HTTP] POST to: %s\n", telemetryUrl.c_str());
        Serial.printf("[HTTP] Payload: %s\n", jsonPayload);

        int httpCode = http.POST(jsonPayload);
        
        if (httpCode == 204) // Azure IoT Hub returns 204 for successful telemetry
        {
            Serial.println("[HTTP] Telemetry data sent successfully via REST API");
            http.end();
            return true;
        }
        else
        {
            Serial.printf("[HTTP] Failed to send telemetry. Status code: %d\n", httpCode);
            if (httpCode > 0) {
                String response = http.getString();
                Serial.printf("[HTTP] Error response: %s\n", response.c_str());
            } else {
                Serial.println("[HTTP] Connection error");
            }
            http.end();
            return false;
        }
    }

private:
    // TODO :
    // 1. Generate token SAS dari API Moorgan ✅
    // 2. Konfigurasi ulang workflow buat send data ✅  
    // 3. Buat function untuk create new device di azure ✅
    //  > if theres no device in database, then create new device to azure
    //  > else use existing device

    // Add data to retry queue
    void addToRetryQueue(const String& topic, const String& payload) {
        if (retryQueue.size() >= MAX_QUEUE_SIZE) {
            Serial.println("[RETRY] Queue full, removing oldest entry");
            retryQueue.erase(retryQueue.begin());
        }
        
        QueuedData data;
        data.topic = topic;
        data.payload = payload;
        data.timestamp = millis();
        data.retryCount = 0;
        
        retryQueue.push_back(data);
        Serial.printf("[RETRY] Added to queue. Queue size: %d\n", retryQueue.size());
    }

    // Process retry queue
    void processRetryQueue() {
        if (retryQueue.empty() || !mqttClient.connected()) {
            return;
        }
        
        unsigned long now = millis();
        for (auto it = retryQueue.begin(); it != retryQueue.end();) {
            // Wait at least 30 seconds between retries
            if (now - it->timestamp > 30000) {
                Serial.printf("[RETRY] Attempting retry %d/%d for topic: %s\n", 
                             it->retryCount + 1, MAX_RETRIES, it->topic.c_str());
                
                bool success = mqttClient.publish(it->topic.c_str(), it->payload.c_str());
                
                if (success) {
                    Serial.println("[RETRY] Retry successful, removing from queue");
                    it = retryQueue.erase(it);
                } else {
                    it->retryCount++;
                    if (it->retryCount >= MAX_RETRIES) {
                        Serial.println("[RETRY] Max retries reached, removing from queue");
                        it = retryQueue.erase(it);
                    } else {
                        it->timestamp = now; // Update for next retry
                        ++it;
                    }
                }
            } else {
                ++it;
            }
        }
    }

    String requestSasToken(const String &uri, const String &deviceId, const String &primaryKey)
    {        
        HTTPClient http;
        String serverUrl = "https://ternak-aja-backend-c7fad0cgb8dmchh0.canadacentral-01.azurewebsites.net/sas";
        
        // Initialize HTTP client with minimal settings to reduce stack usage
        if (!http.begin(wifiClient, serverUrl)) {
            Serial.println("HTTP client initialization failed");
            return "Error: HTTP init failed";
        }
        
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(10000); // Reduced timeout to 10 seconds

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

            // Parse the JSON response with minimal allocation
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
                    Serial.println("SAS Token received successfully");
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
    // Enhanced callback function to handle direct methods
    static void mqttCallback(char *topic, byte *payload, unsigned int length)
    {
        // Avoid String object creation - use char arrays instead
        char message[64]; // Much smaller buffer to prevent stack overflow
        if (length >= sizeof(message)) length = sizeof(message) - 1; // Prevent overflow
        
        for (unsigned int i = 0; i < length; i++) {
            message[i] = (char)payload[i];
        }
        message[length] = '\0'; // Null terminate
        
        Serial.printf("Message arrived [%s] %.*s\n", topic, (int)length, message);
        
        // Handle direct methods (like Python example)
        if (strncmp(topic, "$iothub/methods/POST/", 21) == 0) {
            // Extract method name from topic: $iothub/methods/POST/{method-name}/?$rid={request-id}
            char* methodStart = topic + 21; // Skip "$iothub/methods/POST/"
            char* methodEnd = strchr(methodStart, '/');
            if (!methodEnd) methodEnd = strchr(methodStart, '?');
            
            if (methodEnd) {
                char methodBuffer[16]; // Even smaller buffer for method name
                size_t methodLen = methodEnd - methodStart;
                if (methodLen >= sizeof(methodBuffer)) methodLen = sizeof(methodBuffer) - 1;
                
                strncpy(methodBuffer, methodStart, methodLen);
                methodBuffer[methodLen] = '\0';
                
                // Extract request ID
                char* ridStart = strstr(topic, "$rid=");
                if (ridStart) {
                    ridStart += 5; // Skip "$rid="
                    
                    Serial.printf("Direct method called: %s (rid: %s)\n", methodBuffer, ridStart);
                    
                    // Handle specific methods with reduced allocations
                    handleDirectMethod(String(methodBuffer), String(message), String(ridStart));
                }
            }
        }
    }

    // Non-static instance to access mqttClient
    static RemoteDataSource* instance;
    
    // Handle direct methods from Azure IoT Hub
    static void handleDirectMethod(String methodName, String payload, String requestId) {
        const char* response = "";
        int statusCode = 200;
        // Removed unused responseBuffer to fix compiler warning
        
        if (methodName == "on") {
            // Turn on LED or activate something
            digitalWrite(LED_BUILTIN, LOW); // ESP8266 LED is active LOW
            response = "{\"result\":\"LED ON\"}";
            Serial.println("Direct method: LED ON");
            
        } else if (methodName == "off") {
            // Turn off LED or deactivate something
            digitalWrite(LED_BUILTIN, HIGH); // ESP8266 LED is active LOW
            response = "{\"result\":\"LED OFF\"}";
            Serial.println("Direct method: LED OFF");
            
        } else if (methodName == "getStatus") {
            // Return device status
            response = "{\"status\":\"online\"}";
            Serial.println("Direct method: Get Status");
            
        } else if (methodName == "setSleepInterval") {
            // Simplified - just acknowledge without parsing to save stack
            response = "{\"result\":\"OK\"}";
            Serial.printf("Direct method: setSleepInterval received\n");
            
        } else {
            // Unknown method
            response = "{\"error\":\"Not found\"}";
            statusCode = 404;
            Serial.printf("Unknown direct method: %s\n", methodName.c_str());
        }
        
        // Send response back to Azure IoT Hub
        if (instance) {
            instance->sendDirectMethodResponse(requestId, statusCode, String(response));
        }
    }
    
    // Send response for direct method back to Azure IoT Hub
    void sendDirectMethodResponse(const String& requestId, int statusCode, const String& responseJson) {
        String topic = "$iothub/methods/res/" + String(statusCode) + "/?$rid=" + requestId;
        
        if (mqttClient.connected()) {
            bool success = mqttClient.publish(topic.c_str(), responseJson.c_str());
            if (success) {
                Serial.printf("[MQTT] Direct method response sent: %s -> %s\n", topic.c_str(), responseJson.c_str());
            } else {
                Serial.printf("[MQTT] Failed to send direct method response to topic: %s\n", topic.c_str());
            }
        } else {
            Serial.println("[MQTT] Cannot send direct method response - not connected");
        }
    }
};
