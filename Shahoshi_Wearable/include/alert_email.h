#ifndef ALERT_EMAIL_H
#define ALERT_EMAIL_H

#include <Arduino.h>

void initWiFi();
void sendBrevoEmailAlert(const String& triggerReasons, float accelMag, int soundRaw, float bpm);

#endif // ALERT_EMAIL_H
