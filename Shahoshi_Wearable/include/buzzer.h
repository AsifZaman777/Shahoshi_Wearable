#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

void initBuzzer();
void beep(int freq, int durationMs);
void triggerAlarmBuzzer();

#endif // BUZZER_H
