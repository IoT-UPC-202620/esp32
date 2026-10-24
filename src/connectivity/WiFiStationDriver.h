#ifndef WIFI_STATION_DRIVER_H
#define WIFI_STATION_DRIVER_H

/**
 * @file WiFiStationDriver.h
 * @brief Declares a concrete Wi-Fi station driver built on `WiFiConnectivityDriver`.
 * @ingroup connectivity_module
 */

#include "../ModestIoT/WiFiConnectivityDriver.h"

/**
 * @name Connectivity: Concrete Drivers
 * @ingroup connectivity_module
 * @{
 */

/**
 * @brief Wi-Fi station adapter that completes the `ConnectivityDriver` contract.
 * @ingroup connectivity_module
 *
 * `WiFiConnectivityDriver` leaves `transmit()` unimplemented, so it cannot be instantiated.
 * This driver keeps all of its provisioning and socket behaviour and adds `transmit()` as a
 * one-shot TCP write to a `"host:port"` target, mirroring how `BluetoothConnectivityDriver`
 * writes one line per call. Long-lived protocols (MQTT) keep using `getNetworkSocket()`.
 */
class WiFiStationDriver : public WiFiConnectivityDriver {
private:
    static constexpr size_t HOST_BUFFER_SIZE = 64; ///< Maximum host name length in a target.

public:
    /**
     * @brief Constructs a Wi-Fi station driver.
     * @param ssid Wireless local network access point name.
     * @param password Authentication credential passphrase.
     */
    WiFiStationDriver(const char* ssid, const char* password);

    /**
     * @brief Opens a TCP connection to `target`, writes `data` as one line and closes it.
     * @param target Destination formatted as `"host:port"`.
     * @param data Null-terminated payload.
     * @return `true` when the connection opened and every byte was written.
     */
    bool transmit(const char* target, const char* data) override;
};

/** @} */ // End of Connectivity: Concrete Drivers

#endif // WIFI_STATION_DRIVER_H
