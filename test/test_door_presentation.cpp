#include "minitest.h"
#include "devices/DoorPresentation.h"

static bool operator==(const RgbColor& a, const RgbColor& b) {
    return a.red == b.red && a.green == b.green && a.blue == b.blue;
}

TEST(card_then_granted_opens_and_shows_green_for_2s_in_free_access) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);

    ASSERT_TRUE(dp.onCardRead(1000));
    ASSERT_TRUE(dp.phase() == DoorPhase::WaitingDecision);

    NodeCommand cmd{};
    cmd.type = NodeCommandType::AccessResult;
    cmd.granted = true;
    cmd.durationInMilliseconds = 5000;
    cmd.buzzerPattern = BuzzerPattern::Granted;
    strncpy(cmd.message, "Acceso concedido", sizeof(cmd.message) - 1);

    ASSERT_TRUE(dp.onAccessResult(cmd, 1100));
    ASSERT_TRUE(dp.phase() == DoorPhase::ShowingDecision);

    auto frame1 = dp.tick(1100);
    ASSERT_TRUE(frame1.lockShouldOpen);
    ASSERT_EQ(5000u, frame1.lockDurationInMilliseconds);
    ASSERT_TRUE(frame1.beep);
    ASSERT_TRUE(frame1.beepPattern == BuzzerPattern::Granted);
    ASSERT_FALSE(frame1.ledOn);
    ASSERT_STREQ("Acceso concedido", frame1.headline);

    // One-shot flags cleared
    auto frame2 = dp.tick(1200);
    ASSERT_FALSE(frame2.lockShouldOpen);
    ASSERT_FALSE(frame2.beep);
    ASSERT_FALSE(frame2.ledOn);

    // After 2000 ms from 1100, i.e. at 3100
    auto frame3 = dp.tick(3100);
    ASSERT_TRUE(dp.phase() == DoorPhase::Idle);
    ASSERT_FALSE(frame3.ledOn);
    ASSERT_TRUE(frame3.headline == nullptr);
}

TEST(second_card_while_waiting_is_ignored) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);

    ASSERT_TRUE(dp.onCardRead(1000));
    ASSERT_TRUE(dp.phase() == DoorPhase::WaitingDecision);
    ASSERT_FALSE(dp.onCardRead(2000));
}

TEST(no_result_in_5s_denies_with_gateway_unavailable) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);

    ASSERT_TRUE(dp.onCardRead(1000));
    dp.tick(5999);
    ASSERT_TRUE(dp.phase() == DoorPhase::WaitingDecision);

    auto frame = dp.tick(6000);
    ASSERT_TRUE(dp.phase() == DoorPhase::ShowingDecision);
    ASSERT_FALSE(frame.lockShouldOpen);
    ASSERT_TRUE(frame.beep);
    ASSERT_TRUE(frame.beepPattern == BuzzerPattern::Denied);
    ASSERT_FALSE(frame.ledOn);
    ASSERT_STREQ("Gateway no disponible", frame.headline);
}

TEST(late_result_after_timeout_is_ignored) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);

    ASSERT_TRUE(dp.onCardRead(1000));
    dp.tick(6000); // timed out
    ASSERT_TRUE(dp.phase() == DoorPhase::ShowingDecision);

    NodeCommand cmd{};
    cmd.type = NodeCommandType::AccessResult;
    cmd.granted = true;
    cmd.durationInMilliseconds = 5000;
    ASSERT_FALSE(dp.onAccessResult(cmd, 6500));
}

TEST(reserved_mode_shows_fixed_color_and_RESERVADO) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);

    RgbColor teal{0, 255, 136};
    dp.onMode(DoorMode::Reserved, true, teal);
    ASSERT_TRUE(dp.mode() == DoorMode::Reserved);

    auto frame = dp.tick(1000);
    ASSERT_TRUE(frame.ledColor == teal);
    ASSERT_STREQ("RESERVADO", frame.headline);
}

TEST(rgb_in_free_access_is_rejected) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);
    ASSERT_TRUE(dp.mode() == DoorMode::FreeAccess);

    RgbColor teal{0, 255, 136};
    ASSERT_FALSE(dp.onRgb(teal));
}

TEST(rgb_in_reserved_updates_color) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);

    RgbColor initialColor{10, 20, 30};
    dp.onMode(DoorMode::Reserved, true, initialColor);

    RgbColor newColor{0, 255, 136};
    ASSERT_TRUE(dp.onRgb(newColor));

    auto frame = dp.tick(1000);
    ASSERT_TRUE(frame.ledColor == newColor);
}

TEST(back_to_free_access_turns_led_off) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);

    RgbColor teal{0, 255, 136};
    dp.onMode(DoorMode::Reserved, true, teal);
    dp.onMode(DoorMode::FreeAccess, false, {});

    ASSERT_TRUE(dp.mode() == DoorMode::FreeAccess);
    auto frame = dp.tick(1000);
    ASSERT_FALSE(frame.ledOn);
    ASSERT_TRUE(frame.headline == nullptr);
}

TEST(maintenance_ignores_cards_and_keeps_led_off) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);

    dp.onMaintenance(true, "Under maintenance");
    ASSERT_TRUE(dp.phase() == DoorPhase::Maintenance);
    ASSERT_FALSE(dp.onCardRead(1000));

    auto frame = dp.tick(1000);
    ASSERT_FALSE(frame.ledOn);
    ASSERT_STREQ("Under maintenance", frame.headline);

    dp.onMaintenance(false, "");
    ASSERT_TRUE(dp.phase() == DoorPhase::Idle);
    auto frameAfter = dp.tick(2000);
    ASSERT_FALSE(frameAfter.ledOn);
}

TEST(no_broker_session_outside_reserved_keeps_led_off) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(false);

    auto frame1 = dp.tick(1000);
    ASSERT_FALSE(frame1.ledOn);

    dp.onBrokerSession(true);
    auto frame2 = dp.tick(1000);
    ASSERT_FALSE(frame2.ledOn);
}

TEST(denied_in_reserved_keeps_fixed_color_but_beeps_denied) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);

    RgbColor teal{0, 255, 136};
    dp.onMode(DoorMode::Reserved, true, teal);

    ASSERT_TRUE(dp.onCardRead(1000));
    NodeCommand cmd{};
    cmd.type = NodeCommandType::AccessResult;
    cmd.granted = false;
    cmd.buzzerPattern = BuzzerPattern::Denied;
    strncpy(cmd.message, "Acceso denegado", sizeof(cmd.message) - 1);

    ASSERT_TRUE(dp.onAccessResult(cmd, 1100));

    auto frame = dp.tick(1100);
    // In reserved mode, fixed color is preserved!
    ASSERT_TRUE(frame.ledColor == teal);
    ASSERT_FALSE(frame.lockShouldOpen);
    ASSERT_TRUE(frame.beep);
    ASSERT_TRUE(frame.beepPattern == BuzzerPattern::Denied);
    ASSERT_STREQ("Acceso denegado", frame.headline);
}

TEST(reserved_without_broker_keeps_fixed_color_on) {
    DoorPresentation dp(5000, 2000);
    RgbColor teal{0, 255, 136};
    dp.onMode(DoorMode::Reserved, true, teal);
    dp.onBrokerSession(false);   // the edge keeps enforcing the reservation offline

    auto frame = dp.tick(1000);
    ASSERT_TRUE(frame.ledOn);
    ASSERT_TRUE(frame.ledColor == teal);
}

TEST(reserved_in_maintenance_turns_led_off) {
    DoorPresentation dp(5000, 2000);
    dp.onBrokerSession(true);
    RgbColor teal{0, 255, 136};
    dp.onMode(DoorMode::Reserved, true, teal);
    dp.onMaintenance(true, "Mantenimiento");

    auto frame = dp.tick(1000);
    ASSERT_FALSE(frame.ledOn);
}

TEST(reserved_led_color_is_reported_for_heartbeats) {
    DoorPresentation dp(5000, 2000);
    RgbColor purple{128, 0, 128};
    dp.onMode(DoorMode::Reserved, true, purple);
    RgbColor reported = dp.reservedLedColor();
    ASSERT_EQ(128, reported.red);
    ASSERT_EQ(0, reported.green);
    ASSERT_EQ(128, reported.blue);
}
