#ifndef DOOR_PRESENTATION_H
#define DOOR_PRESENTATION_H

/**
 * @file DoorPresentation.h
 * @brief Pure state machine governing door visual/acoustic feedback, roles and decision timeouts.
 * @ingroup devices_module
 */

#include "../gateway/CommandParser.h"
#include <stdint.h>

/**
 * @brief Phases of the door presentation cycle.
 */
enum class DoorPhase : uint8_t {
    Idle,
    WaitingDecision,
    ShowingDecision,
    Maintenance
};

/**
 * @brief Snapshot of the instantaneous presentation state for actuators.
 */
struct PresentationFrame {
    RgbColor ledColor;
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

public:
    DoorPresentation(uint32_t accessResultTimeoutInMilliseconds, uint32_t decisionFeedbackInMilliseconds);

    bool onCardRead(uint32_t nowInMilliseconds);
    bool onAccessResult(const NodeCommand& command, uint32_t nowInMilliseconds);
    void onMode(DoorMode mode, bool hasColor, RgbColor color);
    bool onRgb(RgbColor color);
    void onMaintenance(bool active, const char* message);
    void onBrokerSession(bool connected);
    PresentationFrame tick(uint32_t nowInMilliseconds);

    DoorMode mode() const;
    DoorPhase phase() const;
};

#endif // DOOR_PRESENTATION_H
