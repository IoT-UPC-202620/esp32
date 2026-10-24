#ifndef RFID_READER_H
#define RFID_READER_H

/**
 * @file RfidReader.h
 * @brief Declares an MFRC522 (RC522) RFID card reader sensor.
 * @ingroup sensors_module
 */

#include "../ModestIoT/Sensor.h"
#include <MFRC522.h>

/**
 * @name Sensors: Access Control
 * @ingroup sensors_module
 * @{
 */

/**
 * @brief SPI RFID reader that emits a card-detected event for every new card presented.
 * @ingroup sensors_module
 *
 * The base `pin` attribute holds the SPI slave-select pin. Each detection caches the card UID
 * and PICC type, then halts the card so it is not read again while it stays on the antenna.
 */
class RfidReader : public Sensor {
private:
    MFRC522 physicalRfidReader; ///< Underlying MFRC522 driver instance.
    byte firmwareVersion; ///< Value of the VersionReg register read at initialization.

    static constexpr size_t UID_BUFFER_SIZE = 32; ///< Up to 10 UID bytes as "XX " plus terminator.
    static constexpr size_t CARD_TYPE_BUFFER_SIZE = 32; ///< Longest PICC type name plus terminator.

    char cachedUid[UID_BUFFER_SIZE]; ///< Last UID, uppercase hex bytes separated by spaces.
    char cachedCompactUid[UID_BUFFER_SIZE]; ///< Last UID without separators.
    char cachedCardType[CARD_TYPE_BUFFER_SIZE]; ///< Last PICC type name.

    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Identifier for propagation suppression.

    /**
     * @brief Formats the UID of the selected card into the cached buffers.
     */
    void cacheSelectedCard();

protected:
    /**
     * @brief Polls the antenna and reads the serial number of any newly presented card.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

public:
    /** @brief Identifier emitted when a new card is read. */
    static constexpr int CARD_DETECTED_EVENT_IDENTIFIER = 60;
    /** @brief Prebuilt event instance for card detection notifications. */
    static const Event CARD_DETECTED_EVENT;

    /**
     * @brief Constructs the reader, starts the SPI bus and initializes the MFRC522.
     * @param slaveSelectGpioPin SPI SS pin (labelled SDA on most modules).
     * @param resetGpioPin Reset pin of the module.
     * @param clockGpioPin SPI SCK pin.
     * @param misoGpioPin SPI MISO pin.
     * @param mosiGpioPin SPI MOSI pin.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    RfidReader(int slaveSelectGpioPin, int resetGpioPin, int clockGpioPin, int misoGpioPin, int mosiGpioPin,
               EventHandler* parentHandler = nullptr);

    /**
     * @brief Reports whether the MFRC522 answered with a valid version register.
     * @return `false` when the register reads 0x00 or 0xFF (wiring or power issue).
     */
    [[nodiscard]] bool isConnected() const;

    /** @brief Returns the version register value read at initialization. */
    [[nodiscard]] byte getFirmwareVersion() const;
    /** @brief Returns the last UID as uppercase hex bytes separated by spaces. */
    [[nodiscard]] const char* getUid() const;
    /** @brief Returns the last UID without separators. */
    [[nodiscard]] const char* getCompactUid() const;
    /** @brief Returns the PICC type name of the last card. */
    [[nodiscard]] const char* getCardType() const;
};

/** @} */ // End of Sensors: Access Control

#endif // RFID_READER_H
