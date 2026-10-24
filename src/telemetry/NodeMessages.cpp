/**
 * @file NodeMessages.cpp
 * @brief Implements the JSON serialization of the node-to-edge messages.
 */

#include "NodeMessages.h"
#include <time.h>

// ---------------- NodeMessage ----------------
void NodeMessage::serializeEnvelope(JsonDocument& serializationDestination) {
    serializationDestination["v"] = CONTRACT_VERSION;

    time_t now = time(nullptr);
    struct tm utc;
    gmtime_r(&now, &utc);
    if (utc.tm_year + 1900 < 2024) {   // clock not synchronized yet: never send a fake timestamp
        serializationDestination["ts"] = nullptr;
        return;
    }
    char isoTimestamp[24];
    strftime(isoTimestamp, sizeof(isoTimestamp), "%Y-%m-%dT%H:%M:%SZ", &utc);
    serializationDestination["ts"] = isoTimestamp;   // ArduinoJson copies char arrays
}

// ---------------- HeartbeatMessage ----------------
HeartbeatMessage::HeartbeatMessage(const char* deviceId, const char* firmware)
    : deviceIdentifier(deviceId), firmwareVersion(firmware) {}

void HeartbeatMessage::serialize(JsonDocument& serializationDestination) const {
    serializeEnvelope(serializationDestination);
    serializationDestination["deviceId"] = deviceIdentifier;
    serializationDestination["fw"] = firmwareVersion;
}

const char* HeartbeatMessage::getChannel() const { return "heartbeat"; }

// ---------------- AccessAttemptMessage ----------------
AccessAttemptMessage::AccessAttemptMessage(const char* compactUid) : credential("") {
    if (compactUid != nullptr) snprintf(credential, CREDENTIAL_BUFFER_SIZE, "%s", compactUid);
}

void AccessAttemptMessage::serialize(JsonDocument& serializationDestination) const {
    serializeEnvelope(serializationDestination);
    serializationDestination["credentialType"] = "RFID";
    serializationDestination["credential"] = credential;
}

const char* AccessAttemptMessage::getChannel() const { return "access"; }

// ---------------- SensorReadingMessage ----------------
SensorReadingMessage::SensorReadingMessage(const char* sensor, float measuredValue, const char* measureUnit)
    : sensorName(sensor), value(measuredValue), unit(measureUnit) {}

void SensorReadingMessage::serialize(JsonDocument& serializationDestination) const {
    serializeEnvelope(serializationDestination);
    serializationDestination["sensor"] = sensorName;
    serializationDestination["value"] = value;
    serializationDestination["unit"] = unit;
}

const char* SensorReadingMessage::getChannel() const { return "readings"; }

// ---------------- CommandAckMessage ----------------
CommandAckMessage::CommandAckMessage(const char* commandId, const char* resultStatus, const char* resultDetail)
    : commandIdentifier(""), status(resultStatus), detail("") {
    if (commandId != nullptr) snprintf(commandIdentifier, TEXT_BUFFER_SIZE, "%s", commandId);
    if (resultDetail != nullptr) snprintf(detail, TEXT_BUFFER_SIZE, "%s", resultDetail);
}

void CommandAckMessage::serialize(JsonDocument& serializationDestination) const {
    serializeEnvelope(serializationDestination);
    serializationDestination["commandId"] = commandIdentifier;
    serializationDestination["status"] = status;
    serializationDestination["detail"] = detail;
}

const char* CommandAckMessage::getChannel() const { return "ack"; }
