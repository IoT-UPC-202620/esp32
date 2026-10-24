/**
 * @file WiFiStationDriver.cpp
 * @brief Implements the one-shot TCP transmit of the Wi-Fi station driver.
 */

#include "WiFiStationDriver.h"
#include <WiFi.h>

WiFiStationDriver::WiFiStationDriver(const char* ssid, const char* password)
    : WiFiConnectivityDriver(ssid, password) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);    // the SDK re-associates by itself after a drop
}

bool WiFiStationDriver::transmit(const char* target, const char* data) {
    if (target == nullptr || data == nullptr || !canTransmit()) {
        return false;
    }

    const char* portSeparator = strrchr(target, ':');
    if (portSeparator == nullptr || static_cast<size_t>(portSeparator - target) >= HOST_BUFFER_SIZE) {
        return false;
    }

    char host[HOST_BUFFER_SIZE];
    size_t hostLength = portSeparator - target;
    memcpy(host, target, hostLength);
    host[hostLength] = '\0';
    auto port = static_cast<uint16_t>(atoi(portSeparator + 1));

    WiFiClient oneShotSocket;
    if (!oneShotSocket.connect(host, port)) {
        return false;
    }
    size_t expectedLength = strlen(data) + 2;   // println appends CR LF
    size_t writtenLength = oneShotSocket.println(data);
    oneShotSocket.stop();
    return writtenLength == expectedLength;
}
