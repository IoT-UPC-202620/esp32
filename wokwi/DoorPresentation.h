#ifndef DOOR_PRESENTATION_H
#define DOOR_PRESENTATION_H

/**
 * @file DoorPresentation.h
 * @brief Pure state machine governing door visual/acoustic feedback, roles and decision timeouts.
 * @ingroup devices_module
 */

#include "CommandParser.h"
#include <stdint.h>

/**
 * @brief Phases of the door presentation cycle.
 */
enum class DoorPhase : uint8_t {
    Idle,
    WaitingDecision,
    ShowingDecision,
    Maintenance,
    Enrolling    // card enrollment mode: the next card is captured, not evaluated
};

/**
 * @brief Snapshot of the instantaneous presentation state for actuators.
 */
struct PresentationFrame {
    bool ledOn;          // false: the RGB LED must be off (only RESERVED lights it)
    RgbColor ledColor;   // meaningful only when ledOn
    const char* headline;
    bool lockShouldOpen;
    uint32_t lockDurationInMilliseconds;
    bool beep;
    BuzzerPattern beepPattern;
};

/**
 * @brief Manages decision feedback, door operational modes and LED colors.
 */
class DoorPresentation {
private:
    uint32_t accessResultTimeoutInMilliseconds;
    uint32_t decisionFeedbackInMilliseconds;

    DoorMode doorMode;
    DoorPhase doorPhase;

    RgbColor reservedColor;
    bool brokerConnected;

    uint32_t waitStartedTimestamp;
    uint32_t decisionStartedTimestamp;
    bool lastDecisionGranted;

    char customHeadline[65];
    char maintenanceMessage[65];

    bool pendingLockOpen;
    uint32_t pendingLockDuration;
    bool pendingBeep;
    BuzzerPattern pendingBeepPattern;
    uint32_t enrollStartedTimestamp;
    uint32_t enrollDurationInMilliseconds;
    bool registered;               // false until the gateway adopts the node (first `mode`)
    char unregisteredHeadline[40]; // "Sin registrar" + "Codigo 1234" on two lines
    bool identifying;
    uint32_t identifyStartedTimestamp;
    uint32_t identifyDurationInMilliseconds;

public:
    DoorPresentation(uint32_t accessResultTimeoutInMilliseconds, uint32_t decisionFeedbackInMilliseconds);

    bool onCardRead(uint32_t nowInMilliseconds);
    bool onAccessResult(const NodeCommand& command, uint32_t nowInMilliseconds);
    void onMode(DoorMode mode, bool hasColor, RgbColor color);
    bool onRgb(RgbColor color);
    void onMaintenance(bool active, const char* message);
    void onBrokerSession(bool connected);
    // Enter (active) or leave enrollment mode; false while in maintenance
    bool onEnroll(bool active, uint32_t durationInMilliseconds, uint32_t nowInMilliseconds);
    // A card read in enrollment mode: true if it must be reported as an enrollment
    // (never as an access attempt); leaves enrollment mode
    bool onEnrollmentCard(uint32_t nowInMilliseconds);
    bool isEnrolling() const;
    // Starts unadopted: cards are ignored and the OLED shows the pairing code until the
    // gateway sends the first `mode`, which is the node's signal that it was adopted
    void requireRegistration(const char* pairingCode);
    bool isRegistered() const;
    // Blinks the RGB LED white for a while and beeps so the admin can spot the board;
    // works whether or not the node is adopted
    void onIdentify(uint32_t durationInMilliseconds, uint32_t nowInMilliseconds);
    PresentationFrame tick(uint32_t nowInMilliseconds);

    DoorMode mode() const;
    RgbColor reservedLedColor() const;   // colour shown while RESERVED (reported in heartbeats)
    DoorPhase phase() const;
};

#endif // DOOR_PRESENTATION_H
