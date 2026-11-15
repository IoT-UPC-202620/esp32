#ifndef EDIFIKA_NODE_H
#define EDIFIKA_NODE_H

/**
 * @file EdifikaNode.h
 * @brief Nodo Edifika (version Wokwi) compuesto con sensores y actuadores de ModestIoT.
 *
 * Igual que el firmware de la placa, salvo:
 *  - pantalla: `LcdStatusDisplay` (CharacterLcdDisplay 20x4) en lugar de la OLED;
 *  - red: `LocalGateway` (gateway simulado por el Monitor serie) en lugar de Wi-Fi + MQTT.
 */

#include <ModestIoT.h>
#include "RfidReader.h"
#include "Relay.h"
#include "RgbLed.h"
#include "DimmableLed.h"
#include "LcdStatusDisplay.h"
#include "LocalGateway.h"
#include "NodeMessages.h"
#include "NodeConfig.h"
#include "DoorPresentation.h"
#include "NodeIdentity.h"
#include "PresenceLight.h"

class EdifikaNode : public Device {
private:
    // El orden de declaracion es el de construccion: el rele primero, cerradura bloqueada al arrancar.
    Relay doorLockRelay;
    Buzzer buzzer;
    RgbLed statusRgbLed;
    Led matrixRowLeds[NodeConfig::MATRIX_ROW_COUNT];
    DimmableLed presenceLights;
    DhtSensor climateSensor;
    RfidReader accessCardReader;   ///< Lector RC522 (MFRC522 de Wokwi).
    UltrasonicSensor presenceSensor;
    LcdStatusDisplay statusDisplay;

    NodeIdentity identity;
    // processQueuedTelemetryData() es const en Device, pero publicar modifica el gateway.
    mutable LocalGateway edgeGateway;

    DoorPresentation doorPresentation;
    PresenceLight presenceLight;

    unsigned long lockOpenedTimestamp;
    unsigned long lockOpenDurationInMilliseconds;
    unsigned long lastHeartbeatTimestamp;
    unsigned long lastAnnounceTimestamp;
    bool brokerSessionWasActive;
    unsigned long lastReadingsTimestamp;
    unsigned long lastRenderTimestamp;
    unsigned long temporaryMessageTimestamp;
    bool temporaryMessageActive;
    char temporaryMessageText[65];
    bool climateAvailable;
    unsigned long lastPresenceLogTimestamp;
    uint32_t distanceReadingsSinceLog;

    void onCardDetected();
    void onClimateRead();
    void onDistanceRead();
    void onDeviceTick();

    void unlockDoor(unsigned long durationInMilliseconds = 0);
    void beep(unsigned long durationInMilliseconds);
    void playBuzzerPattern(BuzzerPattern pattern);
    void showTemporaryMessage(const char* text);
    void updateDoorLock();
    void updateMatrixRows();
    void updatePresenceLights();
    void updateDisplay(const PresentationFrame& frame);
    void processRemoteCommands();
    void updateAnnounce(bool sessionActive, bool sessionJustStarted);
    void updateHeartbeat(bool sessionActive, bool sessionJustStarted);
    void updateReadings();

    void enqueueNodeMessage(NodeMessage* nodeMessage) const;
    static void switchLed(Led& led, bool turnOn);

public:
    static constexpr int DEVICE_TICK_EVENT_IDENTIFIER = 90;
    static constexpr Event DEVICE_TICK_EVENT = Event(DEVICE_TICK_EVENT_IDENTIFIER);

    static constexpr int UNLOCK_DOOR_COMMAND_IDENTIFIER = 400;
    static constexpr Command UNLOCK_DOOR_COMMAND{UNLOCK_DOOR_COMMAND_IDENTIFIER};

    EdifikaNode();

    void begin();

    /** @brief Atiende la consola del gateway simulado. Llamar desde loop(). */
    void maintainNetwork();

    void on(Event event) override;
    void handle(Command command) override;

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override;
};

#endif // EDIFIKA_NODE_H
