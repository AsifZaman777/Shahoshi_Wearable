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
