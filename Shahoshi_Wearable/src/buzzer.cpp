#include "buzzer.h"

#define BUZZER_PIN 25

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
