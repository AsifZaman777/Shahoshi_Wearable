#include "buzzer.h"

// Buzzer pin: D3 (GPIO 4) on XIAO ESP32S3, GPIO 25 on ESP32 DevKit
#ifdef ARDUINO_SEEED_XIAO_ESP32S3
  #define BUZZER_PIN 4
#else
  #define BUZZER_PIN 25
#endif

void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void beep(int freq, int durationMs) {
  tone(BUZZER_PIN, freq);
  delay(durationMs);
  noTone(BUZZER_PIN);
}

void triggerAlarmBuzzer() {
  // Dual-tone emergency siren alarm
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 2500);
    delay(150);
    tone(BUZZER_PIN, 3500);
    delay(150);
  }
  noTone(BUZZER_PIN);
}
