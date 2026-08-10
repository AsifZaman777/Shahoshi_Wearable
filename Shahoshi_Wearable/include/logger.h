#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Logger - outputs all messages to Serial.
// PC-side log files are captured by PlatformIO monitor_filters = log2file
// which writes Serial output to .pio/build/esp32dev/monitor.log

bool initLogger();
void logMessage(const String& message);
void printAllLogsToSerial();

#endif // LOGGER_H
