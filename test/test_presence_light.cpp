#include "minitest.h"
#include "devices/PresenceLight.h"

// threshold 80 cm, 2 consecutive hits, 10 s hold
static PresenceLight makeLight() { return PresenceLight(80.0f, 2, 10000); }

TEST(presence_starts_off) {
    PresenceLight light = makeLight();
    ASSERT_FALSE(light.tick(0));
}

TEST(presence_needs_consecutive_hits_to_turn_on) {
    PresenceLight light = makeLight();
    light.onDistance(50.0f, 100);
    ASSERT_FALSE(light.tick(100));
    light.onDistance(52.0f, 140);
    ASSERT_TRUE(light.tick(140));
}

TEST(presence_far_reading_resets_the_hit_count) {
    PresenceLight light = makeLight();
    light.onDistance(50.0f, 100);
    light.onDistance(200.0f, 140);
    light.onDistance(50.0f, 180);
    ASSERT_FALSE(light.tick(180));
}

TEST(presence_ignores_readings_below_the_sensor_minimum) {
    PresenceLight light = makeLight();
    light.onDistance(0.5f, 100);
    light.onDistance(1.0f, 140);
    ASSERT_FALSE(light.tick(140));
}

TEST(presence_holds_after_the_last_detection_then_turns_off) {
    PresenceLight light = makeLight();
    light.onDistance(50.0f, 0);
    light.onDistance(50.5f, 40);
    ASSERT_TRUE(light.tick(9000));
    light.onDistance(200.0f, 9500);          // the person left: does not switch off at once
    ASSERT_TRUE(light.tick(10039));
    ASSERT_FALSE(light.tick(10040));
}

TEST(presence_new_detection_extends_the_hold) {
    PresenceLight light = makeLight();
    light.onDistance(50.0f, 0);
    light.onDistance(50.5f, 40);
    light.onDistance(51.0f, 8000);
    ASSERT_TRUE(light.tick(17000));
    ASSERT_FALSE(light.tick(18000));
}

TEST(presence_turns_off_without_any_reading) {
    // No echo means UltrasonicSensor emits nothing: only the clock can switch it off
    PresenceLight light = makeLight();
    light.onDistance(50.0f, 0);
    light.onDistance(50.5f, 40);
    ASSERT_FALSE(light.tick(60000));
}
