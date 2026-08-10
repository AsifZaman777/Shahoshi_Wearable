#ifndef HEURISTICS_H
#define HEURISTICS_H

#include <Arduino.h>
#include <Adafruit_Sensor.h>

void evaluateHeuristics(const sensors_event_t& accel, int soundRaw, float bpm);

#endif // HEURISTICS_H
