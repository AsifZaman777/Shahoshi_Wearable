#include "logger.h"

static unsigned long bootTime = 0;

bool initLogger() {
  bootTime = millis();
  Serial.println("=========================================");
  Serial.println("[LOGGER] Log Session Initialized.");
  Serial.println("=========================================");
  return true;
}

void logMessage(const String& message) {
  Serial.println(message);
}

void printAllLogsToSerial() {
  Serial.println("[LOGGER] Logs captured by PlatformIO monitor or log_service.py");
}
