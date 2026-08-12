#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "logger.h"
#include "buzzer.h"
#include "alert_email.h"
#include "gps_handler.h"
#include "heuristics.h"

// ---- XIAO ESP32S3 Pin Mapping (Configuration A from WIRING.txt) ----
#ifdef ARDUINO_SEEED_XIAO_ESP32S3
  #define SDA_PIN      5    // D4 (GPIO 5) - I2C SDA
  #define SCL_PIN      6    // D5 (GPIO 6) - I2C SCL
  #define SOUND_PIN    2    // D1 (GPIO 2 / ADC1) - LM393 Sound Sensor
  #define HR_PIN       3    // D2 (GPIO 3 / ADC1) - MAX30100 Analog Output
#else
  // ---- ESP32 DevKit V1 Pin Mapping (Configuration B from WIRING.txt) ----
  #define SDA_PIN      21   // Default I2C SDA
  #define SCL_PIN      22   // Default I2C SCL
  #define SOUND_PIN    34   // ADC1_CH6
  #define HR_PIN       35   // ADC1_CH7
#endif

Adafruit_MPU6050 mpu;

unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL_MS = 1000;

void printDiagnostics();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ---- Initialize Logger ----
  initLogger();

  logMessage("================================");
  logMessage("SHAHOSHI WEARABLE - MODULAR V2");
  logMessage("================================");

  // ---- Initialize Modules ----
  initWiFi();
  initBuzzer();
  initGPS();

  pinMode(SOUND_PIN, INPUT);
  pinMode(HR_PIN, INPUT);

  // ---- Initialize I2C ----
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(500);

  logMessage("Initializing MPU6050...");
  if (!mpu.begin(0x68, &Wire)) {
    logMessage("[ERROR] MPU6050 not found! Check I2C wiring.");
    for (int i = 0; i < 3; i++) {
      beep(4000, 100);
      delay(100);
    }
    while (true) delay(100);
  }

  logMessage("MPU6050 Connected!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  // Boot Beep
  beep(2000, 200);
  delay(100);
  beep(3000, 200);

  logMessage("SYSTEM READY - Diagnostics active.");
}

void loop() {
  // Feed GPS byte stream / simulation
  feedGPS();

  if (millis() - lastPrint >= PRINT_INTERVAL_MS) {
    lastPrint = millis();
    printDiagnostics();
  }
}

void printDiagnostics() {
  logMessage("--------------------------------------------------");

  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  char mpuBuf[128];
  snprintf(mpuBuf, sizeof(mpuBuf), "MPU6050  | Accel X: %.2f  Y: %.2f  Z: %.2f  Temp: %.2f C",
           accel.acceleration.x, accel.acceleration.y, accel.acceleration.z, temp.temperature);
  logMessage(String(mpuBuf));

  int soundRaw = analogRead(SOUND_PIN);
  logMessage("SOUND    | Raw: " + String(soundRaw));

  int hrRaw = analogRead(HR_PIN);
  float bpm = map(hrRaw, 0, 4095, 40, 180);
  logMessage("HEART RT | BPM: " + String(bpm, 0));

  logMessage("GPS      | " + getGPSLocationStr());

  // Evaluate Emergency Heuristics & Log/Alert
  evaluateHeuristics(accel, soundRaw, bpm);

  logMessage("--------------------------------------------------");
}
