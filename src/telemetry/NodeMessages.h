#ifndef NODE_MESSAGES_H
#define NODE_MESSAGES_H

/**
 * @file NodeMessages.h
 * @brief Declares the node-to-edge MQTT messages as `TelemetryPackage` payloads.
 * @ingroup gateway_module
 *
 * Contract v1 (report section "Contrato MQTT"): every message carries `"v":"1"` and an ISO-8601
 * UTC `ts`, and is published on `edifika/v1/nodes/{deviceId}/{channel}`.
 */

#include "../ModestIoT/TelemetryPackage.h"

/**
 * @name Gateway: Node Messages
 * @ingroup gateway_module
 * @{
 */

/**
 * @brief Base payload that knows its topic channel and writes the common contract fields.
 * @ingroup gateway_module
 */
class NodeMessage : public TelemetryPackage {
protected:
    static constexpr const char* CONTRACT_VERSION = "1"; ///< Value of the `v` field.

    /**
     * @brief Writes `v` and `ts` (or `ts: null` while the clock is not synchronized by NTP).
     * @param serializationDestination Document being filled.
     */
    static void serializeEnvelope(JsonDocument& serializationDestination);

public:
    /**
     * @brief Topic suffix under `edifika/v1/nodes/{deviceId}/`.
     * @return Channel name such as `"readings"` or `"access"`.
     */
    [[nodiscard]] virtual const char* getChannel() const = 0;
};

/**
 * @brief `heartbeat` channel: liveness signal with the firmware version.
 */
class HeartbeatMessage : public NodeMessage {
private:
    const char* deviceIdentifier; ///< Node identifier (static string).
    const char* firmwareVersion; ///< Firmware version (static string).
    const char* doorMode; ///< `"FREE_ACCESS"` or `"RESERVED"` (static string).
    char ledColor[8]; ///< `"#RRGGBB"` while reserved, empty otherwise (serialized as null).

public:
    HeartbeatMessage(const char* deviceId, const char* firmware, const char* currentDoorMode, const char* currentLedColor);
    void serialize(JsonDocument& serializationDestination) const override;
    [[nodiscard]] const char* getChannel() const override;
};

/**
 * @brief `access` channel: credential presented at the reader.
 */
class AccessAttemptMessage : public NodeMessage {
private:
    static constexpr size_t CREDENTIAL_BUFFER_SIZE = 24; ///< Up to 10 UID bytes as hex.
    char credential[CREDENTIAL_BUFFER_SIZE]; ///< Compact card UID.

public:
    explicit AccessAttemptMessage(const char* compactUid);
    void serialize(JsonDocument& serializationDestination) const override;
    [[nodiscard]] const char* getChannel() const override;
};

/**
 * @brief `readings` channel: one sensor value with its unit.
 */
class SensorReadingMessage : public NodeMessage {
private:
    const char* sensorName; ///< Sensor kind (static string), e.g. `"humidity"`.
    float value; ///< Measured value.
    const char* unit; ///< Unit of measure (static string), e.g. `"%"`.

public:
    SensorReadingMessage(const char* sensor, float measuredValue, const char* measureUnit);
    void serialize(JsonDocument& serializationDestination) const override;
    [[nodiscard]] const char* getChannel() const override;
};

/**
 * @brief `ack` channel: result of a command received from the edge.
 */
class CommandAckMessage : public NodeMessage {
private:
    static constexpr size_t TEXT_BUFFER_SIZE = 40; ///< Capacity of each text field.
    char commandIdentifier[TEXT_BUFFER_SIZE]; ///< `commandId` echoed from the command.
    const char* status; ///< `"OK"` or `"ERROR"` (static string).
    char detail[TEXT_BUFFER_SIZE]; ///< Optional human-readable detail.

public:
    CommandAckMessage(const char* commandId, const char* resultStatus, const char* resultDetail = "");
    void serialize(JsonDocument& serializationDestination) const override;
    [[nodiscard]] const char* getChannel() const override;
};

/** @} */ // End of Gateway: Node Messages

#endif // NODE_MESSAGES_H
