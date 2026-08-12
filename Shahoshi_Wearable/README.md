# Shahoshi Wearable Emergency Response System

A smart IoT wearable emergency detection system built on ESP32 microcontrollers. It features real-time sensor processing, heuristic-based anomaly/emergency detection (fall/impact, distress sound/screaming, cardiac arrhythmia), local audible siren alarms, date-based filesystem logging (`LittleFS`), and automated email alert dispatch via the Brevo (Sendinblue) HTTPS REST API.

---

## 📊 Heuristic Thresholds Table

| Category           | Sensor / Source               | Threshold Rule                                               | Evaluated State                 | Action Triggered                     |
| :----------------- | :---------------------------- | :----------------------------------------------------------- | :------------------------------ | :----------------------------------- |
| **Fall / Impact**  | MPU6050 Accelerometer         | $\sqrt{a_x^2 + a_y^2 + a_z^2} > 25.0 \text{ m/s}^2$ (~2.55g) | Hard impact or sudden fall      | Siren Alarm + Brevo Email + File Log |
| **Distress Sound** | LM393 Sound Sensor (Pin 34)   | Analog Reading $> 800$ (out of 4095)                         | Screaming / Loud distress noise | Siren Alarm + Brevo Email + File Log |
| **Bradycardia**    | MAX30100 HR Stand-in (Pin 35) | $\text{Heart Rate} < 45 \text{ BPM}$                         | Cardiac anomaly / Collapse      | Siren Alarm + Brevo Email + File Log |
| **Tachycardia**    | MAX30100 HR Stand-in (Pin 35) | $\text{Heart Rate} > 130 \text{ BPM}$                        | Extreme distress / Panic        | Siren Alarm + Brevo Email + File Log |

---

## 📦 Bill of Materials (BOM) & Estimated Cost

The hardware components have been optimized for a compact wearable footprint and lower total assembly cost:

| Component | Model / Specs | Form Factor / Size | Unit Cost (BDT ৳) | Notes & Advantages |
| :--- | :--- | :--- | :--- | :--- |
| **Microcontroller (Option A - Recommended)** | **Seeed Studio XIAO ESP32S3** | **21 × 17.5 mm** *(Thumb-Sized)* | **৳550 - ৳750** | **Ideal for Wearables**; Integrated Li-Po charging circuit, Wi-Fi + BLE 5.0, Dual-Core 240MHz, 8MB Flash |
| **Microcontroller (Option B)** | ESP32 DevKit V1 (ESP-WROOM-32) | 28.5 × 51.8 mm | ৳420 - ৳480 | Standard dev board, Wi-Fi + BLE, Dual-Core 240MHz, 4MB Flash |
| **GPS Module** | **ATGM336H** (BDS/GPS Dual-Mode) | **13 × 16 mm** *(Ultra-Compact)* | **৳400 - ৳600** | **Upgraded from NEO-6M (22×30mm)**; 9600 Baud UART, NMEA output |
| **IMU / Accelerometer** | MPU6050 6-Axis Motion Sensor | 15 × 20 mm | ৳140 - ৳180 | I2C interface, impact & free-fall detection |
| **Sound / Acoustic Sensor** | LM393 High-Sensitivity Microphone | 15 × 32 mm | ৳60 - ৳90 | Analog audio sensing for screaming detection |
| **Heart Rate / SpO2 Sensor** | MAX30100 PPG Optical Sensor | 14 × 19 mm | ৳250 - ৳320 | I2C pulse rate & cardiac arrhythmia tracker |
| **Audio Alert Siren** | Active Piezo Buzzer (5V / 3.3V) | 12 mm Diameter | ৳25 - ৳40 | Dual-tone local emergency alarm output |
| **Power Management** | TP4056 Charger / Integrated XIAO Battery Pad | 3.7V Li-Po (800mAh) | ৳280 - ৳360 | Portable power module; direct battery connection on XIAO |
| **Misc & Enclosure** | PCB, Jumper Wires, 3D Wearable Case | Custom Wrist/Pendant | ৳125 - ৳180 | Compact wristband/pendant housing |
| **TOTAL SYSTEM COST** | **Complete Wearable Unit** | **Ultra-Compact Footprint** | **৳1,700 - ৳1,950** | **Extremely small footprint using XIAO ESP32S3 + ATGM336H** |

> 📌 **Hardware Architecture & Simulation Notice**:
> - **Wokwi Virtual Simulator (`esp32dev`)**: Uses standard **ESP32 DevKit V1** (`board-esp32-devkit-c-v4`) for web-based simulation to guarantee 100% crash-free FreeRTOS/ISR execution in Wokwi.
> - **Physical Wearable Target (`seeed_xiao_esp32s3`)**: Uses the thumb-sized **Seeed Studio XIAO ESP32S3** for real-world compact wearable deployment.

---

## 🔌 Hardware Pin Mapping & Circuit Diagrams

### 1. Seeed Studio XIAO ESP32S3 (Real Physical Hardware Target)

```
                       +-------------------------------+
                       |  Seeed Studio XIAO ESP32S3    |
                       |      (Thumb-Sized MCU)        |
  LM393 Sound Sensor <---| D1 (GPIO 2)            3V3/5V |---> VCC (All Modules)
 MAX30100 Heart Rate <---| D2 (GPIO 3)             GND |---> GND (Common Ground)
  Piezo Siren Alarm <---| D3 (GPIO 4)     D7 (GPIO 44) |<--- ATGM336H GPS TX (Serial1 RX)
    MPU6050 SDA (I2C) <---| D4 (GPIO 5)     D6 (GPIO 43) |---> ATGM336H GPS RX (Serial1 TX)
    MPU6050 SCL (I2C) <---| D5 (GPIO 6)                 |
                       +-------------------------------+
```

### 2. ESP32 DevKit V1 (Wokwi Virtual Simulator Target)

```
                       +-------------------------------+
                       |       ESP32 DevKit V1         |
                       |    (Wokwi Simulator Board)    |
    MPU6050 SDA (I2C) <---| GPIO 21                3V3 |---> VCC (All Modules)
    MPU6050 SCL (I2C) <---| GPIO 22                GND |---> GND (Common Ground)
  Piezo Siren Alarm <---| GPIO 25            GPIO 16 |<--- GPS TX (Serial2 RX2)
  LM393 Sound Sensor <---| GPIO 34            GPIO 17 |---> GPS RX (Serial2 TX2)
 MAX30100 Heart Rate <---| GPIO 35                            |
                       +-------------------------------+
```

### Pin Comparison Table

| Sensor / Subsystem | Function | Seeed Studio XIAO ESP32S3 (Hardware) | ESP32 DevKit V1 (Wokwi Simulator) |
| :--- | :--- | :--- | :--- |
| **MPU6050 IMU** | I2C Data (SDA) | **D4 (GPIO 5)** | **GPIO 21** |
| **MPU6050 IMU** | I2C Clock (SCL) | **D5 (GPIO 6)** | **GPIO 22** |
| **LM393 Sound Sensor** | Analog Input (Screaming/Distress) | **D1 (GPIO 2)** | **GPIO 34** |
| **MAX30100 Heart Rate**| Analog Input (Pulse Stand-in) | **D2 (GPIO 3)** | **GPIO 35** |
| **Piezo Siren Buzzer** | Digital Output (Dual-Tone Alarm) | **D3 (GPIO 4)** | **GPIO 25** |
| **ATGM336H / GPS** | UART Receive (MCU RX) | **D7 (GPIO 44 / Serial1)** | **GPIO 16 (RX2 / Serial2)** |
| **ATGM336H / GPS** | UART Transmit (MCU TX) | **D6 (GPIO 43 / Serial1)** | **GPIO 17 (TX2 / Serial2)** |

> 📌 Full step-by-step wiring instructions are also preserved in [WIRING.txt](file:///f:/Shahoshi_Wearable/Shahoshi_Wearable/WIRING.txt).

---

## 🚀 How to Build and Run

### 1. Prerequisites

- [VS Code](https://code.visualstudio.com/) with [PlatformIO Extension](https://platformio.org/) installed.
- Python 3.x installed.
- **RISC-V Toolchain** (required for `seeed_xiao_esp32s3` target — install manually to avoid mirror SSL errors):
  ```bash
  pio pkg install --global --tool "platformio/toolchain-riscv32-esp"
  ```

### 2. Environment Setup (`.env`)

Copy `.env.example` to `.env` if you haven't already:

```bash
cp .env.example .env
```

Open `.env` and configure your credentials:

```env
BREVO_API_KEY=xkeysib-.......
ALERT_SENDER_NAME="Shahoshi Wearable System"
ALERT_SENDER_EMAIL=[EMAIL_ADDRESS]
ALERT_RECIPIENT_NAME="Emergency Contact"
ALERT_RECIPIENT_EMAIL=[EMAIL_ADDRESS]
WIFI_SSID=Wokwi-GUEST
WIFI_PASS=
```

_(Note: `.env` and `include/config.h` are protected by `.gitignore` to prevent secret key leakage on GitHub.)_

### 3. Build & Day-Wise Log Service

- **To Build Firmware**:
  ```powershell
  .\build
  ```

- **To Run Day-Wise Live Monitor Log Service**:
  ```powershell
  python log_service.py COM3
  ```
  *(Captures live serial stream and automatically organizes logs day-by-day into `logs/log_YYYY-MM-DD.log`)*

### 4. Run in Wokwi Simulator

1. Open the project in VS Code with the **Wokwi Extension** installed.
2. Press `F1` $\rightarrow$ Select **Wokwi: Start Simulator** (or run `wokwi.toml`).
3. **Simulate Emergency Triggers**:
   - Turn the `pot_sound` knob $>800$ to trigger distress noise alert.
   - Turn the `pot_hr` knob $>130$ BPM or $<45$ BPM to trigger a heart rate alert.
   - Move/shake the `mpu` accelerometer node.
4. **Observe Results**:
   - Hear the dual-tone siren on `BUZZER_PIN` (GPIO 25).
   - View Serial console logs confirming Brevo HTTP response `201`.
   - Check recipient inbox (`asifzaman3123@gmail.com`) for the emergency alert email with Google Maps location links!

### 5. Flash to Real Hardware

1. Connect your ESP32 board via USB.
2. Update `WIFI_SSID` and `WIFI_PASS` in `.env` to your 2.4GHz Wi-Fi credentials.
3. If using real ATGM336H (or NEO-6M) GPS module, set `#define GPS_SIMULATION 0` in `src/gps_handler.cpp`.
4. Upload firmware:
   ```powershell
   & "C:\Users\asif.zaman\.platformio\penv\Scripts\platformio.exe" run --target upload
   ```
5. Monitor serial logs:
   ```powershell
   & "C:\Users\asif.zaman\.platformio\penv\Scripts\platformio.exe" device monitor
   ```

---

## 📁 Modular Project Structure

```
Shahoshi_Wearable/
├── platformio.ini         # PlatformIO build configuration & extra_scripts
├── load_env.py            # Pre-build Python script parsing .env into C++ headers
├── .env / .env.example    # Environment variables (git-ignored)
├── .gitignore             # Ignore rules for .env, build outputs, and local logs
├── include/
│   ├── config.h           # Auto-generated from .env (git-ignored)
│   ├── logger.h           # LittleFS date-formatted logger interface
│   ├── buzzer.h           # Siren & beep audio driver interface
│   ├── alert_email.h      # Brevo HTTPS REST API notification client
│   ├── gps_handler.h      # TinyGPS++ NMEA parser & location formatting
│   └── heuristics.h       # Emergency heuristic evaluation engine
└── src/
    ├── main.cpp           # Modular application setup and loop orchestrator
    ├── logger.cpp         # LittleFS log storage (/logs/log_YYYY-MM-DD.log)
    ├── buzzer.cpp         # Dual-tone alarm siren generator
    ├── alert_email.cpp    # Brevo HTTP POST payload builder & dispatcher
    ├── gps_handler.cpp    # GPS UART2 & simulation parser
    └── heuristics.cpp     # Fall/Impact, Sound, and Cardiac rule evaluator
```

---

## 📑 Date-Formatted Log System

All diagnostic and emergency log entries are preserved on the ESP32 internal flash memory using `LittleFS`.

- **Log File Path**: `/logs/log_YYYY-MM-DD.log` (e.g. `/logs/log_2026-08-10.log`).
- **Entry Format**: `[HH:MM:SS] <Log Message>`
- **On-Boot Log Stream**: On every boot/reset, `logger.cpp` dumps all saved LittleFS logs to the Serial Monitor.
- **Automatic PC File Log Capture**: With `monitor_filters = log2file, time, default` enabled in `platformio.ini`, opening the PlatformIO Serial Monitor automatically captures every device log line directly into local `.log` files in your project directory.


