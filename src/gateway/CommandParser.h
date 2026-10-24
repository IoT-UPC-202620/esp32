#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

/**
 * @file CommandParser.h
 * @brief Declares data structures and functions to parse edge MQTT commands.
 * @ingroup gateway_module
 */

#include <ArduinoJson.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Command types supported by the Edifika IoT contract (v1).
 */
enum class NodeCommandType : uint8_t {
    AccessResult,
    Mode,
    Rgb,
    Unlock,
    Display,
    Buzzer,
    Alert,
    Maintenance,
    TimeSync,
    Unsupported,
    Invalid
};

/**
 * @brief Buzzer acoustic feedback patterns.
 */
enum class BuzzerPattern : uint8_t {
    Granted,
    Denied,
    Alert,
    Off
};

/**
 * @brief Operational access mode of the door node.
 */
enum class DoorMode : uint8_t {
    FreeAccess,
    Reserved
};

/**
 * @brief RGB color represented as 8-bit red, green and blue components.
 */
struct RgbColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

/**
 * @brief Typed representation of an incoming remote command.
 */
struct NodeCommand {
    char commandId[40];              ///< Correlation id ("" when absent).
    NodeCommandType type;            ///< Identified command type.
    bool granted;                    ///< access_result decision.
    char message[65];                ///< Notification text (access_result / display / alert / maintenance).
    char holderName[33];             ///< access_result.name.
    uint32_t durationInMilliseconds; ///< access_result.lockMs, unlock.durationMs / duration_ms.
    BuzzerPattern buzzerPattern;     ///< Pattern to sound on buzzer.
    DoorMode mode;                   ///< Door operational mode.
    bool hasColor;                   ///< True when a color was specified.
    RgbColor color;                  ///< Parsed RGB color.
    bool active;                     ///< alert.active, maintenance.active.
    uint32_t epochSeconds;           ///< time_sync.epoch.
    char error[32];                  ///< Error details if Invalid: "invalid json", "invalid color", "missing params".
};

namespace CommandParser {
    /**
     * @brief Parses an incoming JSON payload into a typed NodeCommand.
     * @param payload Raw byte array of the incoming MQTT packet.
     * @param length Number of bytes in the payload.
     * @return Fully parsed NodeCommand.
     */
    NodeCommand parse(const uint8_t* payload, size_t length);

    /**
     * @brief Parses a hex color string like "#RRGGBB" (case-insensitive).
     * @param text String starting with '#' followed by 6 hexadecimal characters.
     * @param destination Target struct where red, green, blue will be stored.
     * @return True if parsing succeeded, false if format is invalid.
     */
    bool parseHexColor(const char* text, RgbColor& destination);

    /**
     * @brief Formats a color as "#RRGGBB" (uppercase), the inverse of parseHexColor.
     * @param color Color to format.
     * @param destination Buffer of at least 8 characters (7 plus terminator).
     */
    void formatHexColor(const RgbColor& color, char* destination);
}

#endif // COMMAND_PARSER_H
