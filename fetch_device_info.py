import serial
import time
import json

def fetch_device_info(port="COM3", baudrate=115200, command="INFO", timeout=2, read_duration=3):
    """
    Sends a command to the serial device and reads the JSON response.

    Args:
        port (str): Serial port, e.g., "COM3".
        baudrate (int): Baud rate for serial communication.
        command (str): Command string to send.
        timeout (float): Serial read timeout in seconds.
        read_duration (float): Duration to read incoming lines after command.

    Returns:
        dict or None: Parsed JSON object if successful, otherwise None.
    """
    try:
        print(f"[INFO] Connecting to {port} at {baudrate} baud...")
        ser = serial.Serial(port, baudrate, timeout=timeout)
        time.sleep(2)  # Give the ESP8266 time to reset if needed

        # Send command
        print(f"[INFO] Sending command: {command}")
        ser.write((command + "\n").encode("utf-8"))

        # Read response lines
        lines = []
        start_time = time.time()
        while time.time() - start_time < read_duration:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if line:
                print("[SERIAL]", line)
                lines.append(line)

        ser.close()

        # Extract JSON from lines
        full_text = "\n".join(lines)
        start = full_text.find('{')
        if start != -1:
            json_text = full_text[start:]
            return json.loads(json_text)
        else:
            print("⚠️ No JSON found in serial output.")
            return None

    except Exception as e:
        print("❌ Serial error:", e)
        return None


# Example usage
if __name__ == "__main__":
    data = fetch_device_info(port="COM3")
    if data:
        print("\n✅ JSON Data Received:")
        print(json.dumps(data, indent=2))
    else:
        print("\n🚫 No data received or JSON parsing failed.")