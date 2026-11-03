/**
 * @file NodeMessages.cpp
 * @brief Implements the JSON serialization of the node-to-edge messages.
 */

#include "NodeMessages.h"
#include <time.h>
#include <string.h>

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
HeartbeatMessage::HeartbeatMessage(const char* deviceId, const char* firmware, const char* currentDoorMode,
                                   const char* currentLedColor)
    : deviceIdentifier(deviceId), firmwareVersion(firmware), doorMode(currentDoorMode), ledColor("") {
    if (currentLedColor != nullptr) snprintf(ledColor, sizeof(ledColor), "%s", currentLedColor);
}

void HeartbeatMessage::serialize(JsonDocument& serializationDestination) const {
    serializeEnvelope(serializationDestination);
    serializationDestination["deviceId"] = deviceIdentifier;
    serializationDestination["fw"] = firmwareVersion;
    // What the door shows right now: the gateway re-sends `mode` when this is stale
    serializationDestination["mode"] = doorMode;
    if (ledColor[0] != '\0') serializationDestination["color"] = ledColor;
    else serializationDestination["color"] = nullptr;
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

// ---------------- EnrollmentReadMessage ----------------
EnrollmentReadMessage::EnrollmentReadMessage(const char* compactUid) : credential("") {
    if (compactUid != nullptr) snprintf(credential, CREDENTIAL_BUFFER_SIZE, "%s", compactUid);
}

void EnrollmentReadMessage::serialize(JsonDocument& serializationDestination) const {
    serializeEnvelope(serializationDestination);
    serializationDestination["credentialType"] = "RFID";
    serializationDestination["credential"] = credential;
}

const char* EnrollmentReadMessage::getChannel() const { return "enrollment"; }

// ---------------- AnnounceMessage ----------------
AnnounceMessage::AnnounceMessage(const char* deviceId, const char* mac, const char* firmware, const char* model,
                                 const char* code, bool rfid, const char* ip, int rssi)
    : deviceIdentifier(deviceId), macAddress(mac), firmwareVersion(firmware), deviceModel(model),
      pairingCode(code), rfidDetected(rfid), ipAddress(""), signalStrength(rssi) {
    if (ip != nullptr) snprintf(ipAddress, sizeof(ipAddress), "%s", ip);
}

void AnnounceMessage::serialize(JsonDocument& serializationDestination) const {
    serializeEnvelope(serializationDestination);
    serializationDestination["deviceId"] = deviceIdentifier;
    serializationDestination["mac"] = macAddress;
    serializationDestination["fw"] = firmwareVersion;
    serializationDestination["model"] = deviceModel;
    // The RC522 is detected at boot; the outputs and the DHT11 are part of the door node build
    JsonArray capabilities = serializationDestination["capabilities"].to<JsonArray>();
    if (rfidDetected) capabilities.add("RFID");
    capabilities.add("LOCK");
    capabilities.add("BUZZER");
    capabilities.add("OLED");
    capabilities.add("RGB_LED");
    capabilities.add("TEMPERATURE");
    capabilities.add("HUMIDITY");
    if (ipAddress[0] != '\0') serializationDestination["ip"] = ipAddress;
    else serializationDestination["ip"] = nullptr;
    serializationDestination["rssi"] = signalStrength;
    serializationDestination["pairingCode"] = pairingCode;
}

const char* AnnounceMessage::getChannel() const { return "announce"; }

// ---------------- SensorReadingMessage ----------------
SensorReadingMessage::SensorReadingMessage(const char* sensor, float measuredValue, const char* measureUnit)
    : sensorName(sensor), value(measuredValue), unit(measureUnit) {
    if (measureUnit != nullptr && strcmp(measureUnit, "C") == 0) {
        unit = "°C";
    }
}

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
