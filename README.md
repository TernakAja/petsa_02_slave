
# PETSA-02 (Slave)
Source code fungsi-fungsi, programming logic, untuk device slave PETSA-02, berikut adalah penjelasan masing-masing usecase yang terdapat pada device ini.

## 1. Sensor Logic 
Logic Sequence paling awal dalam fungsi device ini.

### :heart_exclamation: - ppgSensor.ino 
Modul ini sebagai parameter Utama untuk mendapatkan Kesehatan Objek **(Detak Jantung)** , kami menggunakan device **MAX30105**.
- Saat initialState alias *initializePPG()*, device akan me-maksimalkan cahaya LED untuk penetrasi Maksimal, karena khususnya ketebalan kulit kuping objek (Hewan) lebih tebal dari kulit jari manusia.
- Gunakan *getHeartRate()* untuk membaca detak jantung objek secara Live, teknologi bio-sensing yang kami adaptasi untuk mendapatkan data adalah dengan mencari & mengkomputasi-kan frekuensi yang muncul dari device , device ini menggunakan metode Photoplethysmography: Dimana device tersebut akan mencari momen tertentu saat aliran darah berubah & menciptakan sebuah pattern (BPM) atau detak setiap menit.

### :thermometer: - infrared_sensor.ino
Modul ini terdapat fungsi untuk mendapatkan Tempratur / Suhu Objek, aspek ini juga penting sebagai Parameter utama terhadap kesehatan Objek, di modul ini kami menggunakan sensor **MLX90614**.
- Saat initialState, kami menyesuaikan Emmisivity berdasarkan referensi khusus mengenai ketebalan objek tersebut, sebagai contoh untuk manusia menggunakan *0.95* sampai *0.97*.
- Fungsi *getTemprature()* akan memuat data suhu tubuh secara Threading atau terus menerus, 


## 2. Data Logic :
```
TODO : 
1. Create a logic to communicate and provide necessary data between *SLAVE*  > *MASTER*.
2. Create timing sequence to send data, as example provide array of 5 or 10 data each 5 minutes
3. Manage power-cycle necessarily, and reduce power consuming as much as possible.
```



