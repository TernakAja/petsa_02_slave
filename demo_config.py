#!/usr/bin/env python3
"""
Comprehensive ESP8266 Configuration Demo Script
Demonstrates all configuration management features
"""

import serial
import time
import json

def connect_to_device(port='COM3', baudrate=115200):
    """Connect to ESP8266 device"""
    try:
        ser = serial.Serial(port, baudrate, timeout=5)
        time.sleep(2)
        ser.reset_input_buffer()
        print(f"✅ Connected to ESP8266 on {port}")
        return ser
    except Exception as e:
        print(f"❌ Failed to connect: {e}")
        return None

def send_command(ser, command, wait_time=1):
    """Send command and read response"""
    print(f"\n🔧 Sending: {command}")
    ser.write(f"{command}\n".encode())
    ser.flush()
    
    time.sleep(wait_time)
    responses = []
    
    while ser.in_waiting > 0:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            responses.append(line)
            print(f"📥 {line}")
    
    return responses

def get_device_info(ser):
    """Get current device information"""
    print("\n📋 Getting current device information...")
    return send_command(ser, "INFO", 2)

def demo_configuration():
    """Demonstrate all configuration features"""
    ser = connect_to_device()
    if not ser:
        return
    
    try:
        # Step 1: Get initial configuration
        print("\n" + "="*60)
        print("STEP 1: Initial Configuration")
        print("="*60)
        get_device_info(ser)
        
        # Step 2: Update device settings
        print("\n" + "="*60)
        print("STEP 2: Updating Device Settings")
        print("="*60)
        
        send_command(ser, "SET_DEVICE:DEVICE_NAME:SmartSensor-01")
        send_command(ser, "SET_DEVICE:LOCATION:Living Room")
        send_command(ser, "SET_DEVICE:INSTALLATION_DATE:2024-01-15")
        
        # Step 3: Update WiFi settings
        print("\n" + "="*60)
        print("STEP 3: Updating WiFi Settings")
        print("="*60)
        
        send_command(ser, "SET_WIFI:HomeNetwork:SecurePassword123")
        
        # Step 4: Save configuration
        print("\n" + "="*60)
        print("STEP 4: Saving Configuration to EEPROM")
        print("="*60)
        
        send_command(ser, "SAVE_CONFIG", 2)
        
        # Step 5: Verify updated configuration
        print("\n" + "="*60)
        print("STEP 5: Verifying Updated Configuration")
        print("="*60)
        
        get_device_info(ser)
        
        # Step 6: Test reset functionality
        print("\n" + "="*60)
        print("STEP 6: Testing Reset to Defaults")
        print("="*60)
        
        send_command(ser, "RESET_CONFIG", 2)
        get_device_info(ser)
        
        # Step 7: Restore our custom settings
        print("\n" + "="*60)
        print("STEP 7: Restoring Custom Settings")
        print("="*60)
        
        send_command(ser, "SET_DEVICE:DEVICE_NAME:SmartSensor-01")
        send_command(ser, "SET_DEVICE:LOCATION:Living Room")
        send_command(ser, "SET_WIFI:HomeNetwork:SecurePassword123")
        send_command(ser, "SAVE_CONFIG", 2)
        
        print("\n" + "="*60)
        print("FINAL CONFIGURATION")
        print("="*60)
        get_device_info(ser)
        
        print("\n✅ Configuration demo completed successfully!")
        print("\n📝 Summary of available commands:")
        print("   • INFO - Get device information")
        print("   • SET_DEVICE:DEVICE_NAME:value - Update device name")
        print("   • SET_DEVICE:LOCATION:value - Update location")
        print("   • SET_DEVICE:INSTALLATION_DATE:value - Update installation date")
        print("   • SET_WIFI:ssid:password - Update WiFi credentials")
        print("   • SAVE_CONFIG - Save settings to EEPROM")
        print("   • RESET_CONFIG - Reset to default values")
        
    except Exception as e:
        print(f"❌ Error during demo: {e}")
    finally:
        ser.close()
        print("\n🔌 Connection closed")

if __name__ == '__main__':
    demo_configuration()