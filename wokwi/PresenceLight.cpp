/**
 * @file PresenceLight.cpp
 * @brief Implements the presence state machine for the white lights.
 */

#include "PresenceLight.h"

PresenceLight::PresenceLight(float thresholdInCentimeters, uint8_t requiredHits, unsigned long holdInMilliseconds)
    : thresholdInCentimeters(thresholdInCentimeters),
      requiredHits(requiredHits > 0 ? requiredHits : 1),
      holdInMilliseconds(holdInMilliseconds),
      consecutiveHits(0),
      lastDetectionTimestamp(0),
      on(false) {}

void PresenceLight::onDistance(float distanceInCentimeters, unsigned long nowInMilliseconds) {
    bool detected = distanceInCentimeters >= MIN_VALID_DISTANCE_IN_CENTIMETERS
        && distanceInCentimeters <= thresholdInCentimeters;
    if (!detected) {
        consecutiveHits = 0;
        return;
    }

    if (consecutiveHits < requiredHits) consecutiveHits++;
    if (consecutiveHits >= requiredHits) {
        on = true;
        lastDetectionTimestamp = nowInMilliseconds;
    }
}

bool PresenceLight::tick(unsigned long nowInMilliseconds) {
    if (on && nowInMilliseconds - lastDetectionTimestamp >= holdInMilliseconds) {
        on = false;
        consecutiveHits = 0;
    }
    return on;
}

bool PresenceLight::isOn() const { return on; }
