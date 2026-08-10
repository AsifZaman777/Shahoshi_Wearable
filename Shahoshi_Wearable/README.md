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

## 🚀 How to Build and Run

### 1. Prerequisites

- [VS Code](https://code.visualstudio.com/) with [PlatformIO Extension](https://platformio.org/) installed.
- Python 3.x installed.

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
3. If using real NEO-6M GPS module, set `#define GPS_SIMULATION 0` in `src/gps_handler.cpp`.
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


