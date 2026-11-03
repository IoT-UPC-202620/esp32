#include "minitest.h"
#include "devices/DoorPresentation.h"
#include "devices/NodeIdentity.h"
#include "gateway/CommandParser.h"

// ---------------- identity ----------------

static const uint8_t SAMPLE_MAC[6] = {0x24, 0x6F, 0x28, 0x3C, 0x71, 0xBF};

TEST(device_id_comes_from_the_last_three_mac_bytes) {
    NodeIdentity identity(SAMPLE_MAC, "");
    ASSERT_STREQ("esp32-3c71bf", identity.deviceId());
    ASSERT_STREQ("24:6F:28:3C:71:BF", identity.mac());
}

TEST(device_id_override_wins_for_boards_registered_before_discovery) {
    NodeIdentity identity(SAMPLE_MAC, "esp32-acc-01");
    ASSERT_STREQ("esp32-acc-01", identity.deviceId());
    ASSERT_STREQ("24:6F:28:3C:71:BF", identity.mac());
}

TEST(pairing_code_is_four_stable_digits_per_board) {
    NodeIdentity first(SAMPLE_MAC, "");
    NodeIdentity again(SAMPLE_MAC, "");
    ASSERT_EQ(4u, std::strlen(first.pairingCode()));
    ASSERT_STREQ(first.pairingCode(), again.pairingCode());
    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(first.pairingCode()[i] >= '0' && first.pairingCode()[i] <= '9');
    }

    const uint8_t otherMac[6] = {0x24, 0x6F, 0x28, 0x3C, 0x71, 0xC0};
    ASSERT_TRUE(NodeIdentity::pairingCodeFor(SAMPLE_MAC) != NodeIdentity::pairingCodeFor(otherMac));
}

// ---------------- identify command ----------------

static NodeCommand parseCommand(const char* json) {
    return CommandParser::parse(reinterpret_cast<const uint8_t*>(json), std::strlen(json));
}

TEST(identify_defaults_to_10_seconds_and_clamps_to_60) {
    auto byDefault = parseCommand(R"({"v":1,"commandId":"i1","type":"identify","params":{},"ts":1})");
    ASSERT_TRUE(byDefault.type == NodeCommandType::Identify);
    ASSERT_EQ(10000u, byDefault.durationInMilliseconds);

    auto tooLong = parseCommand(R"({"v":1,"commandId":"i2","type":"identify","params":{"seconds":600},"ts":1})");
    ASSERT_EQ(60000u, tooLong.durationInMilliseconds);
}

// ---------------- unadopted node ----------------

TEST(unadopted_node_ignores_cards_and_shows_the_pairing_code) {
    DoorPresentation dp(5000, 2000);
    dp.requireRegistration("4821");

    ASSERT_FALSE(dp.isRegistered());
    ASSERT_FALSE(dp.onCardRead(1000));
    ASSERT_FALSE(dp.onEnroll(true, 60000, 1000));

    auto frame = dp.tick(1100);
    ASSERT_STREQ("Sin registrar\nCodigo 4821", frame.headline);
    ASSERT_FALSE(frame.ledOn);
    ASSERT_FALSE(frame.lockShouldOpen);
}

TEST(first_mode_command_adopts_the_node) {
    DoorPresentation dp(5000, 2000);
    dp.requireRegistration("4821");

    dp.onMode(DoorMode::FreeAccess, false, RgbColor{0, 0, 0});

    ASSERT_TRUE(dp.isRegistered());
    ASSERT_TRUE(dp.tick(1000).headline == nullptr);
    ASSERT_TRUE(dp.onCardRead(1100));
}

TEST(identify_blinks_white_and_beeps_even_before_adoption) {
    DoorPresentation dp(5000, 2000);
    dp.requireRegistration("4821");

    dp.onIdentify(1000, 0);
    auto start = dp.tick(0);
    ASSERT_TRUE(start.beep);
    ASSERT_TRUE(start.ledOn);
    ASSERT_EQ(255, start.ledColor.red);
    ASSERT_EQ(255, start.ledColor.green);
    ASSERT_EQ(255, start.ledColor.blue);

    ASSERT_FALSE(dp.tick(300).ledOn);   // off half of every 500 ms
    ASSERT_TRUE(dp.tick(600).ledOn);
    ASSERT_FALSE(dp.tick(1000).ledOn);  // the window passed: back to normal (LED off, not reserved)
    ASSERT_STREQ("Sin registrar\nCodigo 4821", dp.tick(1000).headline);
}
