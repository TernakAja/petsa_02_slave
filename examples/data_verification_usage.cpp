/*
 * Example: How to verify data is actually sent to Azure IoT Hub
 * 
 * This example shows different ways to verify your sensor data
 * reaches Azure IoT Hub successfully.
 */

#include "../src/data/remote_datasource.h"

RemoteDataSource remoteDataSource;

void setup() {
    Serial.begin(115200);
    Serial.println("Azure IoT Hub Data Verification Example");
    
    // Initialize connection
    remoteDataSource.begin();
    
    // Wait for connection
    while (!remoteDataSource.isConnected()) {
        Serial.println("Waiting for Azure IoT Hub connection...");
        remoteDataSource.loop();
        delay(1000);
    }
    
    Serial.println("Connected to Azure IoT Hub!");
    Serial.println("\n" + remoteDataSource.getTransmissionStatus());
}

void loop() {
    // Keep MQTT connection alive
    remoteDataSource.loop();
    
    // Send sensor data every 30 seconds
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 30000) {
        lastSend = millis();
        
        // Simulate sensor readings
        float temperature = 38.5 + random(-20, 20) / 10.0; // 36.5-40.5°C
        float pulseRate = 70 + random(-10, 20);           // 60-90 BPM  
        float spO2 = 98 + random(-3, 2);                  // 95-100%
        
        Serial.println("\n===== SENDING SENSOR DATA =====");
        Serial.printf("Temperature: %.1f°C\n", temperature);
        Serial.printf("Pulse Rate: %.0f BPM\n", pulseRate);
        Serial.printf("SpO2: %.0f%%\n", spO2);
        
        // Send data with verification
        bool success = remoteDataSource.sendData(pulseRate, temperature, spO2);
        
        if (success) {
            Serial.println("✅ Data handed off to TCP layer");
            Serial.println("📡 Check Azure portal to confirm receipt");
        } else {
            Serial.println("❌ Failed to send data");
        }
        
        // Print transmission statistics
        remoteDataSource.printTransmissionStats();
        
        // Print current status
        Serial.println("Current Status: " + remoteDataSource.getTransmissionStatus());
    }
    
    // Print status every 5 minutes
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 300000) { // 5 minutes
        lastStatusPrint = millis();
        Serial.println("\n===== PERIODIC STATUS CHECK =====");
        remoteDataSource.printTransmissionStats();
        
        if (remoteDataSource.isDataLikelyReceived()) {
            Serial.println("🟢 Data transmission appears healthy");
        } else {
            Serial.println("🟡 Data transmission may have issues - check Azure portal");
        }
    }
    
    delay(1000);
}

/*
 * VERIFICATION METHODS:
 * 
 * 1. LOCAL VERIFICATION (what the code provides):
 *    - TCP layer handoff confirmation via mqttClient.publish() return value
 *    - Connection status monitoring
 *    - Transmission statistics (success/failure counts)
 *    - Retry queue monitoring
 * 
 * 2. AZURE IOT HUB VERIFICATION (external tools):
 *    - Azure CLI: az iot hub monitor-events --hub-name <your-hub> --device-id 1
 *    - Azure Portal: IoT Hub > Devices > [Your Device] > Device Telemetry
 *    - Azure Monitor: Check "Telemetry messages sent" metric
 *    - Azure Stream Analytics: Set up real-time data processing
 * 
 * 3. APPLICATION-LEVEL VERIFICATION (advanced):
 *    - Set up Azure Function to receive telemetry
 *    - Have function send cloud-to-device acknowledgment
 *    - Monitor device twin properties for receipt confirmation
 * 
 * IMPORTANT NOTES:
 * - publish() returning true = TCP handoff successful
 * - It does NOT guarantee Azure IoT Hub received the data
 * - Network issues, authentication problems, or Azure service issues
 *   can cause data loss even after successful TCP handoff
 * - Always use external Azure monitoring for true verification
 */
