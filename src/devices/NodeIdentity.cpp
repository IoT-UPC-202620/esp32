/**
 * @file NodeIdentity.cpp
 * @brief Builds the node identity from the chip MAC.
 */

#include "NodeIdentity.h"
#include <stdio.h>

#ifdef ARDUINO
#include <esp_mac.h>
#endif

namespace {
void readFactoryMac(uint8_t mac[NodeIdentity::MAC_LENGTH]) {
#ifdef ARDUINO
    esp_efuse_mac_get_default(mac);
#else
    for (size_t i = 0; i < NodeIdentity::MAC_LENGTH; ++i) mac[i] = 0;
#endif
}
} // namespace

NodeIdentity::NodeIdentity(const char* deviceIdOverride) {
    uint8_t mac[MAC_LENGTH];
    readFactoryMac(mac);
    build(mac, deviceIdOverride);
}

NodeIdentity::NodeIdentity(const uint8_t mac[MAC_LENGTH], const char* deviceIdOverride) {
    build(mac, deviceIdOverride);
}

uint16_t NodeIdentity::pairingCodeFor(const uint8_t mac[MAC_LENGTH]) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < MAC_LENGTH; ++i) {
        hash ^= mac[i];
        hash *= 16777619u;
    }
    return static_cast<uint16_t>(hash % 10000u);
}

void NodeIdentity::build(const uint8_t mac[MAC_LENGTH], const char* deviceIdOverride) {
    if (deviceIdOverride != nullptr && deviceIdOverride[0] != '\0') {
        snprintf(deviceIdText, sizeof(deviceIdText), "%s", deviceIdOverride);
    } else {
        snprintf(deviceIdText, sizeof(deviceIdText), "esp32-%02x%02x%02x", mac[3], mac[4], mac[5]);
    }
    snprintf(macText, sizeof(macText), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(pairingCodeText, sizeof(pairingCodeText), "%04u", static_cast<unsigned>(pairingCodeFor(mac)));
}
