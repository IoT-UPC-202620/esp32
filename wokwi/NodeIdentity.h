#ifndef NODE_IDENTITY_H
#define NODE_IDENTITY_H

/**
 * @file NodeIdentity.h
 * @brief Node identity derived from the chip MAC: one firmware binary serves every board.
 * @ingroup devices_module
 */

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Device id, MAC text and pairing code of this board.
 *
 * - `deviceId`: `esp32-` plus the last three MAC bytes in lowercase hex (`esp32-3c71bf`),
 *   unless an explicit override is given (boards registered before discovery existed).
 * - `pairingCode`: four digits derived from the whole MAC, always the same for a board.
 *   The OLED shows it while the node is not adopted so the admin can match the board.
 */
class NodeIdentity {
public:
    static constexpr size_t MAC_LENGTH = 6;

private:
    char deviceIdText[24];
    char macText[18];
    char pairingCodeText[5];

public:
    /** @brief Identity of this board, read from the factory MAC (ESP32 only). */
    explicit NodeIdentity(const char* deviceIdOverride = "");

    /** @brief Identity for a given MAC (used by the host tests). */
    NodeIdentity(const uint8_t mac[MAC_LENGTH], const char* deviceIdOverride);

    [[nodiscard]] const char* deviceId() const { return deviceIdText; }
    [[nodiscard]] const char* mac() const { return macText; }
    [[nodiscard]] const char* pairingCode() const { return pairingCodeText; }

    /** @brief FNV-1a over the MAC reduced to 0..9999. */
    static uint16_t pairingCodeFor(const uint8_t mac[MAC_LENGTH]);

private:
    void build(const uint8_t mac[MAC_LENGTH], const char* deviceIdOverride);
};

#endif // NODE_IDENTITY_H
