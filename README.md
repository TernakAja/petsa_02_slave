# PETSA-02 Slave

**PETSA-02 Slave** adalah firmware untuk perangkat IoT berbasis ESP8266 (NodeMCU) yang berfungsi sebagai _slave device_ untuk mengumpulkan data kesehatan hewan ternak seperti detak jantung dan suhu tubuh, lalu mengirimkannya ke Azure IoT Hub secara real-time.

## Fitur Utama

- Pembacaan detak jantung dengan sensor **MAX30105**
- Pengukuran suhu tubuh dengan sensor inframerah **MLX90614**
- Pengiriman data ke **Azure IoT Hub** melalui MQTT
- Struktur kode modular dan terorganisir
- Dikembangkan menggunakan **PlatformIO** dengan framework Arduino

## Struktur Proyek

```
src/
├── main.cpp                  // Fungsi utama device
├── utils/
│   ├── sensors.h             // Inisialisasi & pembacaan sensor
│   └── others.h              // Fungsi utilitas tambahan
├── data/
│   └── remote_datasource.h   // Komunikasi dengan Azure IoT Hub
lib/
└── env/
    └── env.h                 // Konfigurasi rahasia dari .env
```

## Setup & Konfigurasi

### 1. Clone Repositori

```bash
git clone https://github.com/TernakAja/petsa_02_slave.git
cd petsa_02_slave
```

### 2. Buat File Konfigurasi `.env.h`

Letakkan di dalam `lib/env/env.h` dan isi dengan konfigurasi Anda:

```cpp
#define WIFI_SSID "nama_wifi"
#define WIFI_PASSWORD "password_wifi"

#define AZURE_IOT_HOST "your-iot-hub.azure-devices.net"
#define AZURE_IOT_PORT 8883
#define AZURE_IOT_DEVICE_ID "device-id-anda"
#define AZURE_IOT_SAS_TOKEN "SharedAccessSignature sr=..."
#define AZURE_IOT_TOPIC "devices/device-id-anda/messages/events/"
```

### 3. Upload ke Board

Gunakan PlatformIO:

```bash
pio run --target upload
```

## Penjelasan Sensor

### MAX30105 (PPG Sensor)

Digunakan untuk mengukur detak jantung dengan metode **Photoplethysmography (PPG)**.  
Sensor membaca pola aliran darah dari perubahan cahaya untuk menghasilkan nilai BPM.

### MLX90614 (IR Thermometer)

Digunakan untuk mengukur suhu tubuh hewan secara non-kontak.  
Emissivity disesuaikan berdasarkan referensi untuk kulit hewan (sekitar 0.98).

## Rencana Pengembangan

- Sinkronisasi data ke perangkat master
- Pengiriman data setiap interval tertentu
- Pengelolaan daya untuk efisiensi baterai
- Implementasi local caching (jika diperlukan)
