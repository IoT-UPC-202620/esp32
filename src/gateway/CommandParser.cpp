/**
 * @file CommandParser.cpp
 * @brief Implements parsing of edge MQTT commands into typed NodeCommand records.
 */

#include "CommandParser.h"
#include <ctype.h>

namespace {

bool iequals(const char* a, const char* b) {
    if (a == nullptr || b == nullptr) return false;
    while (*a != '\0' && *b != '\0') {
        if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b))) {
            return false;
        }
        a++;
        b++;
    }
    return *a == *b;
}

uint8_t parseHexNibble(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(10 + (c - 'A'));
    return 0;
}

uint8_t parseHexByte(const char* s) {
    return static_cast<uint8_t>((parseHexNibble(s[0]) << 4) | parseHexNibble(s[1]));
}

BuzzerPattern parseBuzzerPattern(const char* text) {
    if (iequals(text, "granted")) return BuzzerPattern::Granted;
    if (iequals(text, "denied")) return BuzzerPattern::Denied;
    if (iequals(text, "alert")) return BuzzerPattern::Alert;
    return BuzzerPattern::Off;
}

} // namespace

namespace CommandParser {

bool parseHexColor(const char* text, RgbColor& destination) {
    if (text == nullptr || strlen(text) != 7 || text[0] != '#') {
        return false;
    }
    for (size_t i = 1; i <= 6; ++i) {
        if (!isxdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }
    destination.red = parseHexByte(&text[1]);
    destination.green = parseHexByte(&text[3]);
    destination.blue = parseHexByte(&text[5]);
    return true;
}

NodeCommand parse(const uint8_t* payload, size_t length) {
    NodeCommand cmd{};
    cmd.type = NodeCommandType::Invalid;
    cmd.buzzerPattern = BuzzerPattern::Off;
    cmd.mode = DoorMode::FreeAccess;

    if (payload == nullptr || length == 0) {
        strncpy(cmd.error, "invalid json", sizeof(cmd.error) - 1);
        cmd.error[sizeof(cmd.error) - 1] = '\0';
        return cmd;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err != DeserializationError::Ok) {
        strncpy(cmd.error, "invalid json", sizeof(cmd.error) - 1);
        cmd.error[sizeof(cmd.error) - 1] = '\0';
        return cmd;
    }

    const char* commandId = doc["commandId"] | "";
    strncpy(cmd.commandId, commandId, sizeof(cmd.commandId) - 1);
    cmd.commandId[sizeof(cmd.commandId) - 1] = '\0';

    if (!doc["type"].is<const char*>()) {
        strncpy(cmd.error, "missing type", sizeof(cmd.error) - 1);
        cmd.error[sizeof(cmd.error) - 1] = '\0';
        return cmd;
    }

    const char* typeStr = doc["type"].as<const char*>();
    JsonObject params = doc["params"];

    if (strcmp(typeStr, "access_result") == 0) {
        cmd.type = NodeCommandType::AccessResult;
        const char* resultStr = params["result"] | "";
        cmd.granted = iequals(resultStr, "GRANTED");

        const char* msg = params["message"] | "";
        strncpy(cmd.message, msg, sizeof(cmd.message) - 1);
        cmd.message[sizeof(cmd.message) - 1] = '\0';

        const char* name = params["name"] | "";
        strncpy(cmd.holderName, name, sizeof(cmd.holderName) - 1);
        cmd.holderName[sizeof(cmd.holderName) - 1] = '\0';

        cmd.durationInMilliseconds = params["lockMs"] | 0u;

        const char* buzzerStr = params["buzzer"] | (cmd.granted ? "granted" : "denied");
        cmd.buzzerPattern = parseBuzzerPattern(buzzerStr);
        return cmd;
    }

    if (strcmp(typeStr, "mode") == 0) {
        cmd.type = NodeCommandType::Mode;
        const char* modeStr = params["mode"] | "";
        if (iequals(modeStr, "RESERVED")) {
            cmd.mode = DoorMode::Reserved;
        } else if (iequals(modeStr, "FREE_ACCESS")) {
            cmd.mode = DoorMode::FreeAccess;
        } else {
            cmd.type = NodeCommandType::Invalid;
            strncpy(cmd.error, "missing params", sizeof(cmd.error) - 1);
            cmd.error[sizeof(cmd.error) - 1] = '\0';
            return cmd;
        }

        const char* colorStr = params["color"] | "";
        if (strlen(colorStr) > 0) {
            if (parseHexColor(colorStr, cmd.color)) {
                cmd.hasColor = true;
            } else {
                cmd.type = NodeCommandType::Invalid;
                strncpy(cmd.error, "invalid color", sizeof(cmd.error) - 1);
                cmd.error[sizeof(cmd.error) - 1] = '\0';
                return cmd;
            }
        } else {
            cmd.hasColor = false;
        }
        return cmd;
    }

    if (strcmp(typeStr, "rgb") == 0) {
        cmd.type = NodeCommandType::Rgb;
        const char* colorStr = params["color"] | "";
        if (strlen(colorStr) == 0 || !parseHexColor(colorStr, cmd.color)) {
            cmd.type = NodeCommandType::Invalid;
            strncpy(cmd.error, "invalid color", sizeof(cmd.error) - 1);
            cmd.error[sizeof(cmd.error) - 1] = '\0';
            return cmd;
        }
        cmd.hasColor = true;
        return cmd;
    }

    if (strcmp(typeStr, "unlock") == 0) {
        cmd.type = NodeCommandType::Unlock;
        if (params["durationMs"].is<uint32_t>()) {
            cmd.durationInMilliseconds = params["durationMs"].as<uint32_t>();
        } else if (params["duration_ms"].is<uint32_t>()) {
            cmd.durationInMilliseconds = params["duration_ms"].as<uint32_t>();
        } else {
            cmd.durationInMilliseconds = 0;
        }
        return cmd;
    }

    if (strcmp(typeStr, "display") == 0) {
        cmd.type = NodeCommandType::Display;
        const char* msg = params["message"] | "";
        strncpy(cmd.message, msg, sizeof(cmd.message) - 1);
        cmd.message[sizeof(cmd.message) - 1] = '\0';
        return cmd;
    }

    if (strcmp(typeStr, "buzzer") == 0) {
        cmd.type = NodeCommandType::Buzzer;
        const char* patternStr = params["pattern"] | "off";
        cmd.buzzerPattern = parseBuzzerPattern(patternStr);
        return cmd;
    }

    if (strcmp(typeStr, "alert") == 0) {
        cmd.type = NodeCommandType::Alert;
        cmd.active = params["active"] | false;
        const char* msg = params["message"] | "";
        strncpy(cmd.message, msg, sizeof(cmd.message) - 1);
        cmd.message[sizeof(cmd.message) - 1] = '\0';
        const char* buzzerStr = params["buzzer"] | "alert";
        cmd.buzzerPattern = parseBuzzerPattern(buzzerStr);
        return cmd;
    }

    if (strcmp(typeStr, "maintenance") == 0) {
        cmd.type = NodeCommandType::Maintenance;
        cmd.active = params["active"] | false;
        const char* msg = params["message"] | "";
        strncpy(cmd.message, msg, sizeof(cmd.message) - 1);
        cmd.message[sizeof(cmd.message) - 1] = '\0';
        return cmd;
    }

    if (strcmp(typeStr, "time_sync") == 0) {
        cmd.type = NodeCommandType::TimeSync;
        cmd.epochSeconds = params["epoch"] | 0u;
        return cmd;
    }

    cmd.type = NodeCommandType::Unsupported;
    strncpy(cmd.error, "unsupported command", sizeof(cmd.error) - 1);
    cmd.error[sizeof(cmd.error) - 1] = '\0';
    return cmd;
}

} // namespace CommandParser
