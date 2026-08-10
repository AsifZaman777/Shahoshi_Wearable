#include "heuristics.h"
#include "buzzer.h"
#include "alert_email.h"
#include "logger.h"
#include <math.h>

const float HEURISTIC_ACCEL_IMPACT_THRESHOLD = 25.0; // m/s^2 (~2.55g impact)
const int HEURISTIC_SOUND_THRESHOLD = 800;           // Loud sound threshold
const float HEURISTIC_HR_LOW_THRESHOLD = 45.0;       // Low BPM
const float HEURISTIC_HR_HIGH_THRESHOLD = 130.0;     // High BPM

void evaluateHeuristics(const sensors_event_t& accel, int soundRaw, float bpm) {
  float accelMag = sqrt(accel.acceleration.x * accel.acceleration.x +
                        accel.acceleration.y * accel.acceleration.y +
                        accel.acceleration.z * accel.acceleration.z);

  bool isImpact = (accelMag > HEURISTIC_ACCEL_IMPACT_THRESHOLD);
  bool isLoudSound = (soundRaw > HEURISTIC_SOUND_THRESHOLD);
  bool isAbnormalHR = (bpm < HEURISTIC_HR_LOW_THRESHOLD || bpm > HEURISTIC_HR_HIGH_THRESHOLD);

  if (isImpact || isLoudSound || isAbnormalHR) {
    String triggerReasons = "";
    if (isImpact) {
      triggerReasons += "[Fall/High Impact (" + String(accelMag, 1) + " m/s²)] ";
    }
    if (isLoudSound) {
      triggerReasons += "[Distress Sound (Raw: " + String(soundRaw) + ")] ";
    }
    if (isAbnormalHR) {
      triggerReasons += "[Abnormal Heart Rate (" + String(bpm, 0) + " BPM)] ";
    }

    logMessage("\n**************************************************");
    logMessage("🚨 EMERGENCY HEURISTIC TRIGGERED!");
    logMessage("Reasons: " + triggerReasons);
    logMessage("**************************************************\n");

    // Sound local siren
    triggerAlarmBuzzer();

    // Dispatch email notification via Brevo API
    sendBrevoEmailAlert(triggerReasons, accelMag, soundRaw, bpm);
  }
}
