#include "alert_email.h"
#include "logger.h"
#include "gps_handler.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#if __has_include("config.h")
#include "config.h"
#endif

#ifndef BREVO_API_KEY_VAL
#define BREVO_API_KEY_VAL "YOUR_BREVO_API_KEY"
#define ALERT_SENDER_NAME_VAL "Shahoshi Wearable System"
#define ALERT_SENDER_EMAIL_VAL "zasif4805@gmail.com"
#define ALERT_RECIPIENT_NAME_VAL "Emergency Contact"
#define ALERT_RECIPIENT_EMAIL_VAL "asifzaman3123@gmail.com"
#define WIFI_SSID_VAL "Wokwi-GUEST"
#define WIFI_PASS_VAL ""
#endif

const char* WIFI_SSID = WIFI_SSID_VAL;
const char* WIFI_PASS = WIFI_PASS_VAL;

const char* BREVO_API_KEY = BREVO_API_KEY_VAL;
const char* ALERT_SENDER_NAME = ALERT_SENDER_NAME_VAL;
const char* ALERT_SENDER_EMAIL = ALERT_SENDER_EMAIL_VAL;
const char* ALERT_RECIPIENT_NAME = ALERT_RECIPIENT_NAME_VAL;
const char* ALERT_RECIPIENT_EMAIL = ALERT_RECIPIENT_EMAIL_VAL;

const unsigned long EMAIL_COOLDOWN_MS = 30000;
static unsigned long lastEmailSent = 0;

void initWiFi() {
  logMessage("Connecting to WiFi: " + String(WIFI_SSID));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    logMessage("WiFi Connected! IP: " + WiFi.localIP().toString());
  } else {
    logMessage("WiFi connection pending (reconnecting in background)...");
  }
}

void sendBrevoEmailAlert(const String& triggerReasons, float accelMag, int soundRaw, float bpm) {
  if (WiFi.status() != WL_CONNECTED) {
    logMessage("[ALERT WARN] WiFi disconnected! Cannot send Brevo email.");
    return;
  }

  if (lastEmailSent != 0 && (millis() - lastEmailSent < EMAIL_COOLDOWN_MS)) {
    logMessage("[ALERT WARN] Email rate-limit cooldown active. Skipping duplicate email.");
    return;
  }

  logMessage("[ALERT] Preparing Brevo Email Notification...");

  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification on ESP32

  HTTPClient http;
  if (!http.begin(client, "https://api.brevo.com/v3/smtp/email")) {
    logMessage("[ALERT ERROR] Failed to initialize Brevo HTTP endpoint.");
    return;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("accept", "application/json");
  http.addHeader("api-key", BREVO_API_KEY);

  String locationStr = getGPSLocationStr();
  String mapsUrl = getGoogleMapsUrl();

  String jsonBody = "{";
  jsonBody += "\"sender\":{\"name\":\"" + String(ALERT_SENDER_NAME) + "\",\"email\":\"" + String(ALERT_SENDER_EMAIL) + "\"},";
  jsonBody += "\"to\":[{\"email\":\"" + String(ALERT_RECIPIENT_EMAIL) + "\",\"name\":\"" + String(ALERT_RECIPIENT_NAME) + "\"}],";
  jsonBody += "\"subject\":\"EMERGENCY ALERT: Wearable Device Triggered!\",";

  String htmlStr = "<h2>Shahoshi Wearable Emergency Alert</h2>";
  htmlStr += "<p><b>Trigger Reason:</b> " + triggerReasons + "</p>";
  htmlStr += "<h3>Sensor Telemetry:</h3>";
  htmlStr += "<ul>";
  htmlStr += "<li><b>Acceleration Magnitude:</b> " + String(accelMag, 2) + " m/s²</li>";
  htmlStr += "<li><b>Sound Level (Raw):</b> " + String(soundRaw) + "</li>";
  htmlStr += "<li><b>Heart Rate:</b> " + String(bpm, 0) + " BPM</li>";
  htmlStr += "<li><b>GPS Location:</b> " + locationStr + "</li>";
  if (mapsUrl.length() > 0) {
    htmlStr += "<li><b>Google Maps:</b> <a href='" + mapsUrl + "'>" + mapsUrl + "</a></li>";
  }
  htmlStr += "</ul>";

  jsonBody += "\"htmlContent\":\"" + htmlStr + "\"";
  jsonBody += "}";

  int httpCode = http.POST(jsonBody);

  if (httpCode > 0) {
    logMessage("[ALERT] Brevo HTTP Code: " + String(httpCode));
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      String resp = http.getString();
      logMessage("[ALERT SUCCESS] Email Sent! Response: " + resp);
      lastEmailSent = millis();
    } else {
      String resp = http.getString();
      logMessage("[ALERT FAIL] Response: " + resp);
    }
  } else {
    logMessage("[ALERT ERROR] POST Failed: " + String(http.errorToString(httpCode).c_str()));
  }

  http.end();
}
