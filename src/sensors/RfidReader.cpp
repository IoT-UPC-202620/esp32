/**
 * @file RfidReader.cpp
 * @brief Implements MFRC522 card polling and UID caching.
 */

#include "RfidReader.h"
#include <SPI.h>

const Event RfidReader::CARD_DETECTED_EVENT = Event(RfidReader::CARD_DETECTED_EVENT_IDENTIFIER);

RfidReader::RfidReader(int slaveSelectGpioPin, int resetGpioPin, int clockGpioPin, int misoGpioPin, int mosiGpioPin,
                       EventHandler* parentHandler)
    : Sensor(slaveSelectGpioPin, parentHandler),
      physicalRfidReader(slaveSelectGpioPin, resetGpioPin),
      firmwareVersion(0x00),
      cachedUid("---"),
      cachedCompactUid("---"),
      cachedCardType("")
{
    SPI.begin(clockGpioPin, misoGpioPin, mosiGpioPin, slaveSelectGpioPin);
    physicalRfidReader.PCD_Init();
    firmwareVersion = physicalRfidReader.PCD_ReadRegister(MFRC522::VersionReg);
}

void RfidReader::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        if (physicalRfidReader.PICC_IsNewCardPresent() && physicalRfidReader.PICC_ReadCardSerial()) {
            cacheSelectedCard();

            physicalRfidReader.PICC_HaltA();        // leaves the card idle so it is not read in a loop
            physicalRfidReader.PCD_StopCrypto1();

            event = Event(CARD_DETECTED_EVENT_IDENTIFIER, pin);
            return;
        }
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER; // Suppress
    }
}

void RfidReader::cacheSelectedCard() {
    const MFRC522::Uid& uid = physicalRfidReader.uid;
    size_t spacedLength = 0;
    size_t compactLength = 0;

    for (byte byteIndex = 0; byteIndex < uid.size; byteIndex++) {
        spacedLength += snprintf(cachedUid + spacedLength, UID_BUFFER_SIZE - spacedLength,
                                 byteIndex < uid.size - 1 ? "%02X " : "%02X", uid.uidByte[byteIndex]);
        compactLength += snprintf(cachedCompactUid + compactLength, UID_BUFFER_SIZE - compactLength,
                                  "%02X", uid.uidByte[byteIndex]);
    }

    MFRC522::PICC_Type cardType = MFRC522::PICC_GetType(uid.sak);
    snprintf(cachedCardType, CARD_TYPE_BUFFER_SIZE, "%s",
             reinterpret_cast<const char*>(MFRC522::PICC_GetTypeName(cardType)));
}

bool RfidReader::isConnected() const { return firmwareVersion != 0x00 && firmwareVersion != 0xFF; }
byte RfidReader::getFirmwareVersion() const { return firmwareVersion; }
const char* RfidReader::getUid() const { return cachedUid; }
const char* RfidReader::getCompactUid() const { return cachedCompactUid; }
const char* RfidReader::getCardType() const { return cachedCardType; }
