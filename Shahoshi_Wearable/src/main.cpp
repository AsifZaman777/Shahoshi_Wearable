#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <math.h>

#define SDA_PIN      21
#define SCL_PIN      22
#define BUZZER_PIN   25
#define SOUND_PIN    34   // LM393 analog out (real HW) / potentiometer (sim)
#define HR_PIN       35   // MAX30100 stand-in potentiometer (sim only)
#define GPS_RX_PIN   16   // ESP32 RX2 <- GPS TX (real hardware only)
#define GPS_TX_PIN   17   // ESP32 TX2 -> GPS RX (real hardware only)
#define GPS_BAUD     9600

// 1 = Wokwi simulator build (no GPS wiring, feed canned NMEA sentences
//     into the parser to prove the parsing logic works)
// 0 = real hardware build (read live NMEA data from the NEO-6M over UART2)
#define GPS_SIMULATION 1

#if __has_include("config.h")
#include "config.h"
#endif

#ifndef BREVO_API_KEY_VAL
#define BREVO_API_KEY_VAL "YOUR_BREVO_API_KEY"
#endif

// ---- WiFi Configuration ----
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// ---- Brevo API Configuration ----
const char* BREVO_API_KEY = BREVO_API_KEY_VAL;
const char* ALERT_SENDER_NAME = "Shahoshi Wearable System";
const char* ALERT_SENDER_EMAIL = "zasif4805@gmail.com";
const char* ALERT_RECIPIENT_NAME = "01824500704";
const char* ALERT_RECIPIENT_EMAIL = "asifzaman3123@gmail.com";

// ---- Heuristic Thresholds ----
const float HEURISTIC_ACCEL_IMPACT_THRESHOLD = 25.0; // m/s^2 (~2.55g impact)
const int HEURISTIC_SOUND_THRESHOLD = 800;           // Loud sound / distress threshold
const float HEURISTIC_HR_LOW_THRESHOLD = 45.0;       // Low BPM (bradycardia)
const float HEURISTIC_HR_HIGH_THRESHOLD = 130.0;     // High BPM (tachycardia)

// Rate limiting cooldown (30 seconds between email dispatches)
const unsigned long EMAIL_COOLDOWN_MS = 30000;
unsigned long lastEmailSent = 0;

Adafruit_MPU6050 mpu;
TinyGPSPlus gps;

unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL_MS = 1000;

#if GPS_SIMULATION
// Standard example NMEA sentences - feeding them into TinyGPS++ validates parsing.
// Updated to Dhaka, Bangladesh (23.810333 N, 90.412500 E):
const char* SAMPLE_NMEA[] = {
  "$GPGGA,123519,2348.620,N,09024.750,E,1,08,0.9,545.4,M,46.9,M,,*41\r\n",
  "$GPRMC,123519,A,2348.620,N,09024.750,E,000.0,000.0,100826,000.0,E*72\r\n"
};
const int SAMPLE_NMEA_COUNT = 2;
unsigned long lastGpsFeed = 0;
const unsigned long GPS_FEED_INTERVAL_MS = 2000;
#endif

void printDiagnostics();
void evaluateHeuristics(const sensors_event_t& accel, int soundRaw, float bpm);
void triggerAlarmBuzzer();
void sendBrevoEmailAlert(const String& triggerReasons, float accelMag, int soundRaw, float bpm);

// tone(pin, freq, duration) schedules an internal timer to auto-stop the
// tone, which can race with the LEDC peripheral and print a spurious
// "LEDC is not initialized" error. Calling tone()/noTone() explicitly
// avoids that timer entirely.
void beep(int freq, int durationMs) {
  tone(BUZZER_PIN, freq);
  delay(durationMs);
  noTone(BUZZER_PIN);
}

void triggerAlarmBuzzer() {
  // Dual-tone emergency siren pattern
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 2500);
    delay(150);
    tone(BUZZER_PIN, 3500);
    delay(150);
  }
  noTone(BUZZER_PIN);
}

void sendBrevoEmailAlert(const String& triggerReasons, float accelMag, int soundRaw, float bpm) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ALERT] WiFi not connected! Skipping Brevo email dispatch.");
    return;
  }

  if (lastEmailSent != 0 && (millis() - lastEmailSent < EMAIL_COOLDOWN_MS)) {
    Serial.println("[ALERT] Email cooldown active. Skipping duplicate email dispatch.");
    return;
  }

  Serial.println("[ALERT] Sending Emergency Email via Brevo API...");

  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification on embedded ESP32 client

  HTTPClient http;
  if (!http.begin(client, "https://api.brevo.com/v3/smtp/email")) {
    Serial.println("[ALERT ERROR] Failed to connect to Brevo API endpoint.");
    return;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("accept", "application/json");
  http.addHeader("api-key", BREVO_API_KEY);

  String locationInfo = "Fix Not Available";
  String googleMapsLink = "";
  if (gps.location.isValid()) {
    double lat = gps.location.lat();
    double lng = gps.location.lng();
    locationInfo = "Lat " + String(lat, 6) + ", Lon " + String(lng, 6);
    googleMapsLink = "https://www.google.com/maps?q=" + String(lat, 6) + "," + String(lng, 6);
  }

  String jsonBody = "{";
  jsonBody += "\"sender\":{\"name\":\"" + String(ALERT_SENDER_NAME) + "\",\"email\":\"" + String(ALERT_SENDER_EMAIL) + "\"},";
  jsonBody += "\"to\":[{\"email\":\"" + String(ALERT_RECIPIENT_EMAIL) + "\",\"name\":\"" + String(ALERT_RECIPIENT_NAME) + "\"}],";
  jsonBody += "\"subject\":\"EMERGENCY ALERT: Wearable Device Triggered!\",";

  String htmlStr = "<h2>Shahoshi Wearable Emergency Alert</h2>";
  htmlStr += "<p><b>Trigger Reason:</b> " + triggerReasons + "</p>";
  htmlStr += "<h3>Sensor Metrics:</h3>";
  htmlStr += "<ul>";
  htmlStr += "<li><b>Acceleration Magnitude:</b> " + String(accelMag, 2) + " m/s²</li>";
  htmlStr += "<li><b>Sound Level (Raw):</b> " + String(soundRaw) + "</li>";
  htmlStr += "<li><b>Heart Rate:</b> " + String(bpm, 0) + " BPM</li>";
  htmlStr += "<li><b>GPS Location:</b> " + locationInfo + "</li>";
  if (googleMapsLink.length() > 0) {
    htmlStr += "<li><b>Maps Link:</b> <a href='" + googleMapsLink + "'>" + googleMapsLink + "</a></li>";
  }
  htmlStr += "</ul>";

  jsonBody += "\"htmlContent\":\"" + htmlStr + "\"";
  jsonBody += "}";

  int httpCode = http.POST(jsonBody);

  if (httpCode > 0) {
    Serial.printf("[ALERT] Brevo HTTP Response Code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      String response = http.getString();
      Serial.println("[ALERT SUCCESS] Response: " + response);
      lastEmailSent = millis();
    } else {
      String response = http.getString();
      Serial.println("[ALERT WARN] Response body: " + response);
    }
  } else {
    Serial.printf("[ALERT ERROR] Brevo POST failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

void evaluateHeuristics(const sensors_event_t& accel, int soundRaw, float bpm) {
  float accelMag = sqrt(accel.acceleration.x * accel.acceleration.x +
                        accel.acceleration.y * accel.acceleration.y +
                        accel.acceleration.z * accel.acceleration.z);

  bool isImpact = (accelMag > HEURISTIC_ACCEL_IMPACT_THRESHOLD);
  bool isLoudSound = (soundRaw > HEURISTIC_SOUND_THRESHOLD);
  bool isAbnormalHR = (bpm < HEURISTIC_HR_LOW_THRESHOLD || bpm > HEURISTIC_HR_HIGH_THRESHOLD);

  if (isImpact || isLoudSound || isAbnormalHR) {
    String triggerReasons = "";
    if (isImpact) {
      triggerReasons += "[Fall/High Impact (" + String(accelMag, 1) + " m/s²)] ";
    }
    if (isLoudSound) {
      triggerReasons += "[Distress Sound (Raw: " + String(soundRaw) + ")] ";
    }
    if (isAbnormalHR) {
      triggerReasons += "[Abnormal Heart Rate (" + String(bpm, 0) + " BPM)] ";
    }

    Serial.println("\n**************************************************");
    Serial.println("EMERGENCY HEURISTIC TRIGGERED!");
    Serial.println("Reasons: " + triggerReasons);
    Serial.println("**************************************************\n");

    // Sound alert siren
    triggerAlarmBuzzer();

    // Send email alert via Brevo API
    sendBrevoEmailAlert(triggerReasons, accelMag, soundRaw, bpm);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("SHAHOSHI WEARABLE");
  Serial.println("PHASE 2 - HEURISTIC & ALERT SYSTEM");
  Serial.println("================================");

  // ---- WiFi Connection ----
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 5000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi Connected! IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection pending/timed out (will reconnect in background).");
  }

  // ---- Buzzer ----
  pinMode(BUZZER_PIN, OUTPUT);

  // ---- Sound + heart-rate analog inputs ----
  pinMode(SOUND_PIN, INPUT);
  pinMode(HR_PIN, INPUT);

  // ---- I2C ----
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(500);

  // Scan I2C devices
  Serial.println("Scanning I2C devices...");
  int devicesFound = 0;

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address: 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      devicesFound++;
    }
  }

  if (devicesFound == 0) {
    Serial.println("No I2C devices found!");
  }

  Serial.println();
  Serial.println("Initializing MPU6050...");

  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("ERROR: MPU6050 not found!");
    Serial.println("Check SDA, SCL, VCC and GND connections.");

    // Distinct error tone: three short high beeps
    for (int i = 0; i < 3; i++) {
      beep(4000, 100);
      delay(100);
    }

    while (true) {
      delay(100);
    }
  }

  Serial.println("MPU6050 connected successfully!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  // ---- GPS ----
#if GPS_SIMULATION
  Serial.println("Initializing GPS (SIMULATION - feeding sample NMEA data)...");
#else
  Serial.println("Initializing GPS...");
  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS UART2 started at 9600 baud (RX=GPIO16, TX=GPIO17)");
#endif

  Serial.println("SYSTEM READY");

  // Boot beep
  beep(2000, 200);
  delay(100);
  beep(3000, 200);

  Serial.println();
  Serial.println("Printing all sensor readings every 1s...\n");
}

void loop() {
#if GPS_SIMULATION
  if (millis() - lastGpsFeed >= GPS_FEED_INTERVAL_MS) {
    lastGpsFeed = millis();
    static int sentenceIndex = 0;
    const char* sentence = SAMPLE_NMEA[sentenceIndex];
    for (int i = 0; sentence[i] != '\0'; i++) {
      gps.encode(sentence[i]);
    }
    sentenceIndex = (sentenceIndex + 1) % SAMPLE_NMEA_COUNT;
  }
#else
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }
#endif

  if (millis() - lastPrint >= PRINT_INTERVAL_MS) {
    lastPrint = millis();
    printDiagnostics();
  }
}

void printDiagnostics() {
  Serial.println("--------------------------------------------------");

  // ---- MPU6050 ----
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  Serial.print("MPU6050  | Accel X: ");
  Serial.print(accel.acceleration.x, 2);
  Serial.print("  Y: ");
  Serial.print(accel.acceleration.y, 2);
  Serial.print("  Z: ");
  Serial.print(accel.acceleration.z, 2);
  Serial.print("  (m/s^2, pass: Z ~9.81 at rest, spikes when shaken)");
  Serial.println();

  Serial.print("           Temp: ");
  Serial.print(temp.temperature, 2);
  Serial.println(" C");

  // ---- Sound (LM393 stand-in) ----
  int soundRaw = analogRead(SOUND_PIN);
  Serial.print("SOUND    | Raw: ");
  Serial.print(soundRaw);
  Serial.println("   (pass: <200 quiet, >800 loud - turn the potentiometer)");

  // ---- Heart rate (MAX30100 stand-in) ----
  int hrRaw = analogRead(HR_PIN);
  float bpm = map(hrRaw, 0, 4095, 40, 180);
  Serial.print("HEART RT | Potentiometer BPM: ");
  Serial.print(bpm, 0);
  Serial.println("   (pass: 60-90 rest, 100+ = 'exercise')");

  // ---- GPS ----
  Serial.print("GPS      | ");
  if (gps.location.isValid()) {
    Serial.print("Lat: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print("  Lon: ");
    Serial.print(gps.location.lng(), 6);
    Serial.print("  Sats: ");
    Serial.println(gps.satellites.isValid() ? gps.satellites.value() : 0);
  } else {
    Serial.print("Waiting for fix... (chars processed: ");
    Serial.print(gps.charsProcessed());
    Serial.println(")");
  }

  // ---- Evaluate Heuristics ----
  evaluateHeuristics(accel, soundRaw, bpm);

  Serial.println("--------------------------------------------------");
}

