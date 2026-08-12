#include "gps_handler.h"

// GPS Module Config (Supports ATGM336H 13x16mm ultra-compact module & NEO-6M, 9600 Baud UART)
#define GPS_RX_PIN   16
#define GPS_TX_PIN   17
#define GPS_BAUD     9600
#define GPS_SIMULATION 1

TinyGPSPlus gps;

#if GPS_SIMULATION
const char* SAMPLE_NMEA[] = {
  "$GPGGA,123519,2348.620,N,09024.750,E,1,08,0.9,545.4,M,46.9,M,,*41\r\n",
  "$GPRMC,123519,A,2348.620,N,09024.750,E,000.0,000.0,100826,000.0,E*72\r\n"
};
const int SAMPLE_NMEA_COUNT = 2;
unsigned long lastGpsFeed = 0;
const unsigned long GPS_FEED_INTERVAL_MS = 2000;
#endif

void initGPS() {
#if GPS_SIMULATION
  Serial.println("Initializing GPS (SIMULATION mode)...");
#else
  Serial.println("Initializing GPS Hardware (UART2)...");
  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#endif
}

void feedGPS() {
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
}

String getGPSDateStr() {
  if (gps.date.isValid() && gps.date.year() > 2000) {
    char dateBuf[16];
    snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d",
             gps.date.year(), gps.date.month(), gps.date.day());
    return String(dateBuf);
  }
  return "2026-08-10"; // Default date fallback
}

String getGPSTimeStr() {
  if (gps.time.isValid()) {
    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
             gps.time.hour(), gps.time.minute(), gps.time.second());
    return String(timeBuf);
  }
  unsigned long totalSec = millis() / 1000;
  char timeBuf[16];
  snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu:%02lu",
           (totalSec / 3600) % 24, (totalSec / 60) % 60, totalSec % 60);
  return String(timeBuf);
}

String getGPSLocationStr() {
  if (gps.location.isValid()) {
    return "Lat " + String(gps.location.lat(), 6) + ", Lon " + String(gps.location.lng(), 6);
  }
  return "Fix Not Available";
}

String getGoogleMapsUrl() {
  if (gps.location.isValid()) {
    return "https://www.google.com/maps?q=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
  }
  return "";
}
