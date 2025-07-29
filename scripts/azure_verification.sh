#!/bin/bash

# Azure IoT Hub Data Verification Commands
# Use these commands to verify your ESP8266 data actually reaches Azure IoT Hub

echo "=== Azure IoT Hub Data Verification Tools ==="
echo ""

# Replace these with your actual values
IOT_HUB_NAME="your-iot-hub-name"
DEVICE_ID="1"
RESOURCE_GROUP="your-resource-group"

echo "📋 Prerequisites:"
echo "1. Install Azure CLI: https://docs.microsoft.com/en-us/cli/azure/install-azure-cli"
echo "2. Login: az login"
echo "3. Install IoT extension: az extension add --name azure-iot"
echo ""

echo "🔍 Real-time telemetry monitoring:"
echo "az iot hub monitor-events --hub-name $IOT_HUB_NAME --device-id $DEVICE_ID"
echo ""

echo "📊 Device connection status:"
echo "az iot hub device-identity show --hub-name $IOT_HUB_NAME --device-id $DEVICE_ID --query connectionState"
echo ""

echo "📈 Hub metrics (last 1 hour):"
echo "az monitor metrics list --resource /subscriptions/{subscription-id}/resourceGroups/$RESOURCE_GROUP/providers/Microsoft.Devices/IotHubs/$IOT_HUB_NAME --metric 'd2c.telemetry.ingress.success' --interval PT1H"
echo ""

echo "🔧 Direct method test (turn LED on):"
echo "az iot hub invoke-device-method --hub-name $IOT_HUB_NAME --device-id $DEVICE_ID --method-name 'on'"
echo ""

echo "🔧 Direct method test (get status):"
echo "az iot hub invoke-device-method --hub-name $IOT_HUB_NAME --device-id $DEVICE_ID --method-name 'getStatus'"
echo ""

echo "📱 Device twin properties:"
echo "az iot hub device-twin show --hub-name $IOT_HUB_NAME --device-id $DEVICE_ID"
echo ""

echo "⚠️  Troubleshooting commands:"
echo "# Check device authentication"
echo "az iot hub device-identity show --hub-name $IOT_HUB_NAME --device-id $DEVICE_ID --query 'authentication'"
echo ""
echo "# List all devices"
echo "az iot hub device-identity list --hub-name $IOT_HUB_NAME --query '[].deviceId'"
echo ""
echo "# Check hub connection string"
echo "az iot hub connection-string show --hub-name $IOT_HUB_NAME"
echo ""

echo "🌐 Azure Portal verification:"
echo "1. Go to https://portal.azure.com"
echo "2. Navigate to your IoT Hub"
echo "3. Go to 'Devices' → Select device '$DEVICE_ID'"
echo "4. Click 'Device Telemetry' to see real-time data"
echo "5. Check 'Metrics' tab for transmission statistics"
echo ""

echo "✅ What to look for:"
echo "- Telemetry messages appearing in real-time monitor"
echo "- 'Connected' status in device connection state"  
echo "- Increasing message count in hub metrics"
echo "- Successful direct method responses"
echo ""

echo "❌ Common issues:"
echo "- Device shows as 'Disconnected' → Check WiFi/MQTT connection"
echo "- No telemetry messages → Check SAS token and topic format"
echo "- Authentication errors → Verify device credentials"
echo "- Timeout errors → Check network connectivity"
