#ifndef PRESENCE_LIGHT_H
#define PRESENCE_LIGHT_H

/**
 * @file PresenceLight.h
 * @brief Declares the pure presence state machine that drives the white lights.
 * @ingroup core_module
 */

#include <stdint.h>

/**
 * @brief Decides when the presence lights are on from ultrasonic distance readings.
 * @ingroup core_module
 *
 * Pure logic (no Arduino calls) so it runs in the host tests.
 *
 * - A reading counts as a detection when it is within [MIN_VALID_DISTANCE, threshold].
 * - The lights turn on after `requiredHits` consecutive detections (filters HC-SR04 glitches).
 * - A reading beyond the threshold resets the count, but does not turn the lights off.
 * - The lights turn off `holdInMilliseconds` after the last detection. This is time based
 *   because `UltrasonicSensor` emits nothing when there is no echo or the distance repeats.
 */
class PresenceLight {
private:
    float thresholdInCentimeters;
    uint8_t requiredHits;
    unsigned long holdInMilliseconds;

    uint8_t consecutiveHits;
    unsigned long lastDetectionTimestamp;
    bool on;

public:
    /** @brief Readings below this are sensor noise (the HC-SR04 does not measure under ~2 cm). */
    static constexpr float MIN_VALID_DISTANCE_IN_CENTIMETERS = 2.0f;

    PresenceLight(float thresholdInCentimeters, uint8_t requiredHits, unsigned long holdInMilliseconds);

    /** @brief Feeds a new distance reading taken at `nowInMilliseconds`. */
    void onDistance(float distanceInCentimeters, unsigned long nowInMilliseconds);

    /**
     * @brief Expires the hold time.
     * @return `true` while the lights must be on.
     */
    bool tick(unsigned long nowInMilliseconds);

    [[nodiscard]] bool isOn() const;
};

#endif // PRESENCE_LIGHT_H
