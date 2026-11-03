#include "minitest.h"
#include "devices/DoorPresentation.h"
#include "gateway/CommandParser.h"

// ---------------- enroll command ----------------

static NodeCommand parseEnroll(const char* json) {
    return CommandParser::parse(reinterpret_cast<const uint8_t*>(json), std::strlen(json));
}

TEST(enroll_command_active_with_seconds) {
    auto c = parseEnroll(R"({"v":1,"commandId":"e1","type":"enroll","params":{"active":true,"seconds":60},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Enroll);
    ASSERT_TRUE(c.active);
    ASSERT_EQ(60000u, c.durationInMilliseconds);
    ASSERT_STREQ("e1", c.commandId);
}

TEST(enroll_command_defaults_to_60_seconds_and_clamps_to_300) {
    auto byDefault = parseEnroll(R"({"v":1,"commandId":"e2","type":"enroll","params":{"active":true},"ts":1})");
    ASSERT_EQ(60000u, byDefault.durationInMilliseconds);

    auto tooLong = parseEnroll(R"({"v":1,"commandId":"e3","type":"enroll","params":{"active":true,"seconds":9999},"ts":1})");
    ASSERT_EQ(300000u, tooLong.durationInMilliseconds);
}

TEST(enroll_command_inactive) {
    auto c = parseEnroll(R"({"v":1,"commandId":"e4","type":"enroll","params":{"active":false},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Enroll);
    ASSERT_FALSE(c.active);
}

TEST(enroll_command_without_active_is_invalid) {
    auto c = parseEnroll(R"({"v":1,"commandId":"e5","type":"enroll","params":{"seconds":60},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Invalid);
    ASSERT_STREQ("missing params", c.error);
}

// ---------------- enrollment mode ----------------

TEST(enrollment_mode_shows_headline_and_cyan_led) {
    DoorPresentation dp(5000, 2000);
    ASSERT_TRUE(dp.onEnroll(true, 60000, 1000));
    ASSERT_TRUE(dp.isEnrolling());

    auto frame = dp.tick(1100);
    ASSERT_STREQ("MODO REGISTRO", frame.headline);
    ASSERT_TRUE(frame.ledOn);
    ASSERT_EQ(0, frame.ledColor.red);
    ASSERT_EQ(180, frame.ledColor.green);
    ASSERT_EQ(255, frame.ledColor.blue);
    ASSERT_FALSE(frame.lockShouldOpen);
}

TEST(card_in_enrollment_mode_is_captured_without_opening_the_lock) {
    DoorPresentation dp(5000, 2000);
    dp.onEnroll(true, 60000, 1000);

    ASSERT_TRUE(dp.onEnrollmentCard(5000));
    ASSERT_FALSE(dp.isEnrolling());

    auto frame = dp.tick(5000);
    ASSERT_FALSE(frame.lockShouldOpen);
    ASSERT_TRUE(frame.beep);
    ASSERT_TRUE(frame.beepPattern == BuzzerPattern::Granted);
    ASSERT_STREQ("Tarjeta capturada", frame.headline);

    // Only the first card counts: back to normal access afterwards
    ASSERT_FALSE(dp.onEnrollmentCard(5100));
    auto later = dp.tick(7100);
    ASSERT_TRUE(dp.phase() == DoorPhase::Idle);
    ASSERT_TRUE(later.headline == nullptr);
    ASSERT_TRUE(dp.onCardRead(7200));
}

TEST(enrollment_mode_ends_by_itself_when_the_window_passes) {
    DoorPresentation dp(5000, 2000);
    dp.onEnroll(true, 60000, 1000);

    dp.tick(60999);
    ASSERT_TRUE(dp.isEnrolling());
    dp.tick(61000);
    ASSERT_FALSE(dp.isEnrolling());
    ASSERT_FALSE(dp.onEnrollmentCard(61001));
}

TEST(enrollment_mode_can_be_cancelled) {
    DoorPresentation dp(5000, 2000);
    dp.onEnroll(true, 60000, 1000);
    ASSERT_TRUE(dp.onEnroll(false, 0, 2000));
    ASSERT_FALSE(dp.isEnrolling());
}

TEST(enrollment_mode_is_refused_during_maintenance) {
    DoorPresentation dp(5000, 2000);
    dp.onMaintenance(true, "Mantenimiento");
    ASSERT_FALSE(dp.onEnroll(true, 60000, 1000));
    ASSERT_TRUE(dp.phase() == DoorPhase::Maintenance);
}

TEST(reserved_door_returns_to_its_colour_after_enrollment) {
    DoorPresentation dp(5000, 2000);
    dp.onMode(DoorMode::Reserved, true, RgbColor{255, 255, 0});
    dp.onEnroll(true, 60000, 1000);
    ASSERT_EQ(0, dp.tick(1100).ledColor.red);   // cyan while enrolling

    dp.onEnroll(false, 0, 2000);
    auto frame = dp.tick(2100);
    ASSERT_TRUE(frame.ledOn);
    ASSERT_EQ(255, frame.ledColor.red);
    ASSERT_EQ(255, frame.ledColor.green);
    ASSERT_EQ(0, frame.ledColor.blue);
    ASSERT_STREQ("RESERVADO", frame.headline);
}
