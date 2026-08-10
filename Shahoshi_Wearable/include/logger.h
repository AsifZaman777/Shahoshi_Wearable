#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

bool initLogger();
void logMessage(const String& message);
void printAllLogsToSerial();
String getCurrentDateStr();
String getCurrentTimestampStr();

#endif // LOGGER_H
