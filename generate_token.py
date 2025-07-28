import base64
import hashlib
import hmac
import time
import urllib.parse

def generate_sas_token(uri: str, key: str, expiry: int = 3600):
    """
    Generate a SAS token for Azure IoT Hub device authentication.

    Parameters:
    - uri: The resource URI (typically "<hostname>/devices/<device_id>")
    - key: The base64-encoded primary key
    - expiry: Token time-to-live in seconds (default: 1 hour)

    Returns:
    - SAS token string
    """
    ttl = int(time.time()) + expiry
    sign_key = "%s\n%d" % (urllib.parse.quote(uri, safe='').lower(), ttl)

    decoded_key = base64.b64decode(key)
    signature = hmac.new(
        decoded_key,
        sign_key.encode('utf-8'),
        hashlib.sha256
    ).digest()

    encoded_signature = urllib.parse.quote(
        base64.b64encode(signature), safe=''
    )

    sas_token = (
        f"SharedAccessSignature sr={urllib.parse.quote(uri, safe='').lower()}"
        f"&sig={encoded_signature}"
        f"&se={ttl}"
    )

    return sas_token


# === YOUR CONFIGURATION ===
hostname = "Moorgan-IoT-Hub.azure-devices.net"
device_id = "1"
primary_key = "XdH2RhlnPiJeG5f4TpAphsxYyV8us+BqQLflh3TGZ8Q="

resource_uri = f"{hostname}/devices/{device_id}"

# Generate the SAS token
sas_token = generate_sas_token(resource_uri, primary_key, expiry=3600)

# Output
print("SAS Token:\n")
print(sas_token)