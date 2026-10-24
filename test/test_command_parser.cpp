#include "minitest.h"
#include "gateway/CommandParser.h"

static NodeCommand parse(const char* json) {
    return CommandParser::parse(reinterpret_cast<const uint8_t*>(json), std::strlen(json));
}

TEST(access_result_granted) {
    auto c = parse(R"({"v":1,"commandId":"c1","type":"access_result","params":{"result":"GRANTED","reason":"ok","message":"Access granted","buzzer":"granted","name":"Ana Perez","lockMs":5000},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::AccessResult && c.granted);
    ASSERT_EQ(5000u, c.durationInMilliseconds);
    ASSERT_STREQ("Ana Perez", c.holderName);
    ASSERT_STREQ("Access granted", c.message);
    ASSERT_STREQ("c1", c.commandId);
    ASSERT_TRUE(c.buzzerPattern == BuzzerPattern::Granted);
}

TEST(access_result_denied_has_no_duration) {
    auto c = parse(R"({"v":1,"commandId":"c2","type":"access_result","params":{"result":"DENIED","reason":"delinquent","message":"Acceso denegado","buzzer":"denied"},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::AccessResult);
    ASSERT_FALSE(c.granted);
    ASSERT_EQ(0u, c.durationInMilliseconds);
    ASSERT_STREQ("", c.holderName);
    ASSERT_STREQ("Acceso denegado", c.message);
    ASSERT_TRUE(c.buzzerPattern == BuzzerPattern::Denied);
}

TEST(mode_reserved_with_color) {
    auto c = parse(R"({"v":1,"commandId":"c3","type":"mode","params":{"mode":"RESERVED","color":"#00FF88"},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Mode);
    ASSERT_TRUE(c.mode == DoorMode::Reserved);
    ASSERT_TRUE(c.hasColor);
    ASSERT_EQ(0, c.color.red);
    ASSERT_EQ(255, c.color.green);
    ASSERT_EQ(136, c.color.blue);
}

TEST(mode_free_access_without_color) {
    auto c = parse(R"({"v":1,"commandId":"c4","type":"mode","params":{"mode":"FREE_ACCESS"},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Mode);
    ASSERT_TRUE(c.mode == DoorMode::FreeAccess);
    ASSERT_FALSE(c.hasColor);
}

TEST(rgb_lowercase_is_accepted) {
    auto c = parse(R"({"v":1,"commandId":"c5","type":"rgb","params":{"color":"#00ff88"},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Rgb);
    ASSERT_TRUE(c.hasColor);
    ASSERT_EQ(0, c.color.red);
    ASSERT_EQ(255, c.color.green);
    ASSERT_EQ(136, c.color.blue);
}

TEST(rgb_without_hash_is_invalid_color) {
    auto c = parse(R"({"v":1,"commandId":"c6","type":"rgb","params":{"color":"00FF88"},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Invalid);
    ASSERT_STREQ("invalid color", c.error);
}

TEST(unlock_accepts_camel_and_snake_case) {
    auto c1 = parse(R"({"v":1,"commandId":"c7","type":"unlock","params":{"durationMs":4000},"ts":1})");
    ASSERT_TRUE(c1.type == NodeCommandType::Unlock);
    ASSERT_EQ(4000u, c1.durationInMilliseconds);

    auto c2 = parse(R"({"v":1,"commandId":"c8","type":"unlock","params":{"duration_ms":6000},"ts":1})");
    ASSERT_TRUE(c2.type == NodeCommandType::Unlock);
    ASSERT_EQ(6000u, c2.durationInMilliseconds);
}

TEST(unknown_type_is_unsupported) {
    auto c = parse(R"({"v":1,"commandId":"c9","type":"dance","params":{},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Unsupported);
    ASSERT_STREQ("c9", c.commandId);
}

TEST(invalid_json_is_invalid) {
    auto c = parse("this is definitely not json");
    ASSERT_TRUE(c.type == NodeCommandType::Invalid);
    ASSERT_STREQ("invalid json", c.error);
}

TEST(missing_command_id_is_empty_string) {
    auto c = parse(R"({"v":1,"type":"unlock","params":{"durationMs":3000},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Unlock);
    ASSERT_STREQ("", c.commandId);
}

TEST(time_sync_epoch) {
    auto c = parse(R"({"v":1,"commandId":"c10","type":"time_sync","params":{"epoch":1782000000,"timezone":"America/Lima","driftSeconds":3.1},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::TimeSync);
    ASSERT_EQ(1782000000u, c.epochSeconds);
}

TEST(long_message_is_truncated_to_64_chars) {
    auto c = parse(R"({"v":1,"commandId":"c11","type":"display","params":{"message":"0123456789012345678901234567890123456789012345678901234567890123456789"},"ts":1})");
    ASSERT_TRUE(c.type == NodeCommandType::Display);
    ASSERT_EQ(64u, std::strlen(c.message));
}

TEST(format_hex_color_round_trips_with_parse) {
    char text[8];
    CommandParser::formatHexColor(RgbColor{0, 255, 136}, text);
    ASSERT_STREQ("#00FF88", text);
    RgbColor parsed{};
    ASSERT_TRUE(CommandParser::parseHexColor(text, parsed));
    ASSERT_EQ(0, parsed.red);
    ASSERT_EQ(255, parsed.green);
    ASSERT_EQ(136, parsed.blue);
}
