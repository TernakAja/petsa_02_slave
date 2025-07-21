#!/usr/bin/env python3
"""
Simple test script for ESP8266 configuration
"""

import serial
import time

def test_config():
    try:
        # Connect to ESP8266
        ser = serial.Serial('COM3', 115200, timeout=5)
        time.sleep(2)
        print("✅ Connected to ESP8266")
        
        # Clear buffer
        ser.reset_input_buffer()
        
        # Test 1: Get current info
        print("\n📋 Test 1: Getting current configuration...")
        ser.write(b'INFO\n')
        ser.flush()
        
        # Read response
        for i in range(20):  # Read up to 20 lines
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"📥 {line}")
            time.sleep(0.1)
        
        # Test 2: Update device name
        print("\n🔧 Test 2: Updating device name...")
        ser.write(b'SET_DEVICE:DEVICE_NAME:MyESP8266\n')
        ser.flush()
        
        # Read response
        for i in range(5):
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"📥 {line}")
            time.sleep(0.1)
        
        # Test 3: Update WiFi config
        print("\n🔧 Test 3: Updating WiFi config...")
        ser.write(b'SET_WIFI:MyNetwork:MyPassword123\n')
        ser.flush()
        
        # Read response
        for i in range(5):
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"📥 {line}")
            time.sleep(0.1)
        
        # Test 4: Save configuration
        print("\n💾 Test 4: Saving configuration...")
        ser.write(b'SAVE_CONFIG\n')
        ser.flush()
        
        # Read response
        for i in range(10):
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"📥 {line}")
            time.sleep(0.1)
        
        # Test 5: Get updated info
        print("\n📋 Test 5: Getting updated configuration...")
        ser.write(b'INFO\n')
        ser.flush()
        
        # Read response
        for i in range(20):
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"📥 {line}")
            time.sleep(0.1)
        
        ser.close()
        print("\n✅ Test completed successfully!")
        
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == '__main__':
    test_config()