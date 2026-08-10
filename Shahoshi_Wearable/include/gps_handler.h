#ifndef GPS_HANDLER_H
#define GPS_HANDLER_H

#include <Arduino.h>
#include <TinyGPS++.h>

extern TinyGPSPlus gps;

void initGPS();
void feedGPS();
String getGPSDateStr();
String getGPSTimeStr();
String getGPSLocationStr();
String getGoogleMapsUrl();

#endif // GPS_HANDLER_H
