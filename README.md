---
title: ESP32 Heart Rate Monitor with WiFi Upload

---

# ESP32 Heart Rate Monitor with WiFi Upload

A simple ESP32-based heart rate monitoring system using the MAX30102/MAX30105 sensor, OLED display, LEDs, and buzzer.
The device measures heart rate, displays real-time and averaged BPM, and uploads data to **ThingSpeak** manually (button press) or automatically when high heart rate is detected.

---

## Features

* Real-time heart rate (BPM) measurement
* Moving average heart rate calculation
* OLED display (128×64)
* Visual (LED) and audio (buzzer) alerts
* Manual data upload via button
* Automatic data upload when heart rate exceeds threshold
* Non-blocking WiFi connection handling
* Serial debugging output

---

## Hardware Required

* ESP32 development board (I used an ESP32 devkit v1 but most probably work)
* MAX30105 heart rate sensor
* 128×64 OLED display (SSH1106, I2C)
* Push button
* Buzzer
* 1× Red LED
* 1× Green LED
* 3x Resistors (lowkey anything around 1k - 10k I just used random values cause I don't care about the LED brightness rlly)
* Jumper wires

---

## Wiring
![Screenshot 2025-12-16 232952](https://hackmd.io/_uploads/S1h28jjN-l.png)

I tried to kind of recreate this on Wokwi but it's pretty messy.

### I2C Devices

| Device       | ESP32 Pin |
| ------------ | --------- |
| MAX30105 SDA | GPIO 21   |
| MAX30105 SCL | GPIO 22   |
| OLED SDA     | GPIO 21   |
| OLED SCL     | GPIO 22   |
| GND          | GND       |
| VCC          | 3.3V      |

### Other Components

| Component | ESP32 Pin             |
| --------- | --------------------- |
| Button    | GPIO 4 (INPUT_PULLUP) (with pulldown resistor) |
| Buzzer    | GPIO 2                |
| Red LED   | GPIO 5 (with resistor)                |
| Green LED | GPIO 18 (with resistor)               |

---

## Libraries Used

Install these via **Arduino Library Manager**:

* `MAX30105` (SparkFun)
* `heartRate` (included in MAX30105 library)
* `GyverOLED`
* `WiFi` (ESP32 core)

---

## Software Setup

1. Install **Arduino IDE**
2. Install **ESP32 board support**
3. Select board: **ESP32 Dev Module**
4. Select correct COM port
5. Upload the sketch

---

## Configuration
Make a thingspeak channel and get the API first

### WiFi Credentials

Use your wifi
```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

### ThingSpeak API Key

```cpp
const char* apiKey = "YOUR_THINGSPEAK_API_KEY";
```

### Heart Rate Threshold

```cpp
int high = 95;   // BPM threshold for alert & auto-send
```

### Auto-Send Cooldown

```cpp
const unsigned long sendCooldown = 15000; // 15 seconds
```

---

## How It Works

### Heart Rate Measurement

* Reads IR value from MAX30105
* Detects heart beats using `checkForBeat()`
* Calculates BPM and moving average
* Does all the logic after that

### Display Logic

* OLED shows:

  * Current BPM
  * Average BPM
  * Heart status
  * WiFi status

### Alert System

* **Green LED** → normal heart rate
* **Red LED + buzzer** → high heart rate
* Alerts only trigger after valid calibration

### Data Upload

* **Manual upload**: press button
* **Automatic upload**:

  * Triggered when BPM > threshold
  * Rate-limited to every 15 seconds

---

## WiFi Handling

* Non-blocking WiFi status check every 10 seconds
* Automatic reconnection if disconnected
* Upload disabled when WiFi is offline

---

## Serial Monitor

Baud rate: **115200**

Example output:

```
IR=52340, BPM=78.4, Avg BPM=76, WiFi=Connected
```

---

## Known Limitations

* No physical schematic provided
* Finger pressure affects accuracy
* ThingSpeak uses HTTP (not HTTPS)
* Basic beat detection (not medical-grade)
* Slight delay in sending data
* Free version of thingspeak can only send every 15 seconds

---

## Future Improvements
* HTTPS support
* Battery
* SD card logging
* Mobile dashboard
* Calibration screen

---
