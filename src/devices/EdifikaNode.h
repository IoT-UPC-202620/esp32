#ifndef EDIFIKA_NODE_H
#define EDIFIKA_NODE_H

/**
 * @file EdifikaNode.h
 * @brief Declares the Edifika access/climate node composed from ModestIoT sensors and actuators.
 * @ingroup core_module
 */

#include "../ModestIoT/Device.h"
#include "../ModestIoT/DhtSensor.h"
#include "../ModestIoT/Buzzer.h"
#include "../ModestIoT/Led.h"
#include "../sensors/RfidReader.h"
#include "../actuators/Relay.h"
#include "../actuators/RgbLed.h"
#include "../actuators/OledDisplay.h"
#include "../connectivity/WiFiStationDriver.h"
#include "../gateway/EdgeMqttGatewayClient.h"
#include "../telemetry/NodeMessages.h"
#include "../config/NodeConfig.h"
#include "DoorPresentation.h"

/**
 * @brief ESP32 node: RFID access with an electric lock, DHT11 climate, OLED, buzzer,
 *        status RGB LED and a 3x3 LED matrix driven row by row.
 * @ingroup core_module
 *
 * Sensors are polled by the `Device` round-robin scheduler and propagate their events to
 * this device, which reacts by issuing commands to the actuators. The device also registers
 * itself in the scheduler with `DEVICE_TICK_EVENT` to run the time-based behaviours
 * (lock auto-relock, presentation tick, matrix rows and display refresh) without blocking.
 */
class EdifikaNode : public Device {
private:
    // Declaration order is construction order: the relay goes first so the lock stays closed at boot.
    Relay doorLockRelay; ///< Electric lock relay (contacts COM/NO).
    Buzzer buzzer; ///< Passive buzzer for audible feedback.
    RgbLed statusRgbLed; ///< Status RGB LED driven by DoorPresentation.
    Led matrixRowLeds[NodeConfig::MATRIX_ROW_COUNT]; ///< One NPN-switched row of the 3x3 matrix each.
    DhtSensor climateSensor; ///< DHT11 temperature and humidity sensor.
    RfidReader accessCardReader; ///< RC522 access card reader.
    OledDisplay statusDisplay; ///< SSD1306 status display.

    WiFiStationDriver wifiDriver; ///< Wi-Fi link (ModestIoT `WiFiConnectivityDriver`).
    // processQueuedTelemetryData() is const in Device, but publishing mutates the client.
    mutable EdgeMqttGatewayClient edgeGateway; ///< MQTT link with the Edge Gateway.

    DoorPresentation doorPresentation; ///< Pure state machine for decision, mode and LED.

    unsigned long lockOpenedTimestamp; ///< When the lock was last energized.
    unsigned long lockOpenDurationInMilliseconds; ///< How long the current opening lasts.
    unsigned long lastHeartbeatTimestamp; ///< When the last heartbeat was queued.
    unsigned long lastReadingsTimestamp; ///< When periodic readings were last queued.
    unsigned long lastRenderTimestamp; ///< When the display was last refreshed.
    unsigned long temporaryMessageTimestamp; ///< When a display/alert message was started.
    bool temporaryMessageActive; ///< `true` while a display/alert message is active.
    char temporaryMessageText[65]; ///< Content of the display/alert message.
    bool climateAvailable; ///< `false` until the first DHT reading arrives.

    void onCardDetected();
    void onClimateRead();
    void onDeviceTick();

    void unlockDoor(unsigned long durationInMilliseconds = 0);
    void beep(unsigned long durationInMilliseconds);
    void playBuzzerPattern(BuzzerPattern pattern);
    void updateDoorLock();
    void updateMatrixRows();
    void updateDisplay(const PresentationFrame& frame);
    void processRemoteCommands();
    void updateHeartbeat();
    void updateReadings();

    /**
     * @brief Hands a message to the Device telemetry queue (deleted if the queue is full).
     */
    void enqueueNodeMessage(NodeMessage* nodeMessage) const;

    /**
     * @brief Sends TURN_ON/TURN_OFF to an LED only when its state must change.
     */
    static void switchLed(Led& led, bool turnOn);

public:
    /** @brief Self-scheduled event that drives the time-based behaviours. */
    static constexpr int DEVICE_TICK_EVENT_IDENTIFIER = 90;
    static constexpr Event DEVICE_TICK_EVENT = Event(DEVICE_TICK_EVENT_IDENTIFIER);

    /** @brief Command that opens the lock for `LOCK_OPEN_DURATION_IN_MILLISECONDS`. */
    static constexpr int UNLOCK_DOOR_COMMAND_IDENTIFIER = 400;
    static constexpr Command UNLOCK_DOOR_COMMAND{UNLOCK_DOOR_COMMAND_IDENTIFIER};

    /**
     * @brief Builds every peripheral; the lock relay is set to its safe state first.
     */
    EdifikaNode();

    /**
     * @brief Runs the startup checks, registers the scheduler slots and starts the engine.
     */
    void begin();

    /**
     * @brief Keeps Wi-Fi and the MQTT session alive. Call it from `loop()`.
     */
    void maintainNetwork();

    void on(Event event) override;
    void handle(Command command) override;

protected:
    /**
     * @brief Publishes queued messages on the NetworkWorker task.
     * Only `NodeMessage` instances are ever enqueued by this device.
     */
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override;
};

#endif // EDIFIKA_NODE_H
