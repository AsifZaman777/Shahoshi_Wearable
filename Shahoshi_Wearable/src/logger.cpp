#include "logger.h"
#include "gps_handler.h"

static bool littleFsReady = false;

bool initLogger() {
  if (!LittleFS.begin(true)) {
    Serial.println("[LOGGER ERROR] LittleFS Mount Failed!");
    littleFsReady = false;
    return false;
  }
  
  littleFsReady = true;
  if (!LittleFS.exists("/logs")) {
    LittleFS.mkdir("/logs");
  }
  
  logMessage("=========================================");
  logMessage("System Started - Log Session Initialized");
  logMessage("=========================================");
  return true;
}

String getCurrentDateStr() {
  return getGPSDateStr();
}

String getCurrentTimestampStr() {
  return getGPSTimeStr();
}

void logMessage(const String& message) {
  // Output to Serial console
  Serial.println(message);

  if (!littleFsReady) {
    return;
  }

  String dateStr = getCurrentDateStr();
  String logDirPath = "/logs";
  String logFilePath = logDirPath + "/log_" + dateStr + ".log";

  if (!LittleFS.exists(logDirPath)) {
    LittleFS.mkdir(logDirPath);
  }

  File file = LittleFS.open(logFilePath, FILE_APPEND);
  if (file) {
    String formattedLine = "[" + getCurrentTimestampStr() + "] " + message + "\n";
    file.print(formattedLine);
    file.close();
  }
}

void printAllLogsToSerial() {
  if (!littleFsReady) {
    Serial.println("[LOGGER] LittleFS not mounted.");
    return;
  }

  if (!LittleFS.exists("/logs")) {
    Serial.println("[LOGGER] No /logs directory found.");
    return;
  }

  File root = LittleFS.open("/logs");
  if (!root || !root.isDirectory()) {
    Serial.println("[LOGGER] Failed to open /logs directory.");
    return;
  }

  Serial.println("\n========== PRESERVED LITTLEFS LOG FILES ==========");
  File file = root.openNextFile();
  bool foundAny = false;
  while (file) {
    foundAny = true;
    Serial.printf("\n--- Log File: %s (%d bytes) ---\n", file.name(), file.size());
    while (file.available()) {
      Serial.write(file.read());
    }
    file = root.openNextFile();
  }
  if (!foundAny) {
    Serial.println("No log files present yet.");
  }
  Serial.println("\n==================================================\n");
}
