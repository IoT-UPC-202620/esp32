/**
 * @file DoorPresentation.cpp
 * @brief Implements pure state machine governing door visual/acoustic feedback, roles and timeouts.
 */

#include "DoorPresentation.h"
#include <string.h>

namespace {
constexpr RgbColor COLOR_WHITE{255, 255, 255};
constexpr RgbColor COLOR_GREEN{0, 255, 0};
constexpr RgbColor COLOR_RED{255, 0, 0};
constexpr RgbColor COLOR_AMBER{255, 128, 0};
} // namespace

DoorPresentation::DoorPresentation(uint32_t accessResultTimeoutInMilliseconds, uint32_t decisionFeedbackInMilliseconds)
    : accessResultTimeoutInMilliseconds(accessResultTimeoutInMilliseconds),
      decisionFeedbackInMilliseconds(decisionFeedbackInMilliseconds),
      doorMode(DoorMode::FreeAccess),
      doorPhase(DoorPhase::Idle),
      reservedColor(COLOR_WHITE),
      brokerConnected(false),
      waitStartedTimestamp(0),
      decisionStartedTimestamp(0),
      lastDecisionGranted(false),
      pendingLockOpen(false),
      pendingLockDuration(0),
      pendingBeep(false),
      pendingBeepPattern(BuzzerPattern::Off)
{
    customHeadline[0] = '\0';
    maintenanceMessage[0] = '\0';
}

bool DoorPresentation::onCardRead(uint32_t nowInMilliseconds) {
    if (doorPhase == DoorPhase::WaitingDecision || doorPhase == DoorPhase::Maintenance) {
        return false;
    }
    doorPhase = DoorPhase::WaitingDecision;
    waitStartedTimestamp = nowInMilliseconds;
    strncpy(customHeadline, "Validando...", sizeof(customHeadline) - 1);
    customHeadline[sizeof(customHeadline) - 1] = '\0';
    return true;
}

bool DoorPresentation::onAccessResult(const NodeCommand& command, uint32_t nowInMilliseconds) {
    if (doorPhase != DoorPhase::WaitingDecision) {
        return false;
    }
    doorPhase = DoorPhase::ShowingDecision;
    decisionStartedTimestamp = nowInMilliseconds;
    lastDecisionGranted = command.granted;

    if (command.granted) {
        pendingLockOpen = true;
        pendingLockDuration = command.durationInMilliseconds;
        pendingBeep = true;
        pendingBeepPattern = command.buzzerPattern;
        if (command.message[0] != '\0') {
            strncpy(customHeadline, command.message, sizeof(customHeadline) - 1);
        } else {
            strncpy(customHeadline, "Acceso concedido", sizeof(customHeadline) - 1);
        }
    } else {
        pendingLockOpen = false;
        pendingLockDuration = 0;
        pendingBeep = true;
        pendingBeepPattern = command.buzzerPattern;
        if (command.message[0] != '\0') {
            strncpy(customHeadline, command.message, sizeof(customHeadline) - 1);
        } else {
            strncpy(customHeadline, "Acceso denegado", sizeof(customHeadline) - 1);
        }
    }
    customHeadline[sizeof(customHeadline) - 1] = '\0';
    return true;
}

void DoorPresentation::onMode(DoorMode mode, bool hasColor, RgbColor color) {
    doorMode = mode;
    if (doorMode == DoorMode::Reserved) {
        if (hasColor) {
            reservedColor = color;
        }
    }
}

bool DoorPresentation::onRgb(RgbColor color) {
    if (doorMode == DoorMode::FreeAccess) {
        return false;
    }
    reservedColor = color;
    return true;
}

void DoorPresentation::onMaintenance(bool active, const char* message) {
    if (active) {
        doorPhase = DoorPhase::Maintenance;
        if (message != nullptr && message[0] != '\0') {
            strncpy(maintenanceMessage, message, sizeof(maintenanceMessage) - 1);
        } else {
            strncpy(maintenanceMessage, "Mantenimiento", sizeof(maintenanceMessage) - 1);
        }
        maintenanceMessage[sizeof(maintenanceMessage) - 1] = '\0';
    } else {
        if (doorPhase == DoorPhase::Maintenance) {
            doorPhase = DoorPhase::Idle;
        }
    }
}

void DoorPresentation::onBrokerSession(bool connected) {
    brokerConnected = connected;
}

PresentationFrame DoorPresentation::tick(uint32_t nowInMilliseconds) {
    if (doorPhase == DoorPhase::WaitingDecision) {
        if (nowInMilliseconds - waitStartedTimestamp >= accessResultTimeoutInMilliseconds) {
            doorPhase = DoorPhase::ShowingDecision;
            decisionStartedTimestamp = nowInMilliseconds;
            lastDecisionGranted = false;
            pendingLockOpen = false;
            pendingLockDuration = 0;
            pendingBeep = true;
            pendingBeepPattern = BuzzerPattern::Denied;
            strncpy(customHeadline, "Gateway no disponible", sizeof(customHeadline) - 1);
            customHeadline[sizeof(customHeadline) - 1] = '\0';
        }
    } else if (doorPhase == DoorPhase::ShowingDecision) {
        if (nowInMilliseconds - decisionStartedTimestamp >= decisionFeedbackInMilliseconds) {
            doorPhase = DoorPhase::Idle;
        }
    }

    PresentationFrame frame{};
    // One-shot actions
    frame.lockShouldOpen = pendingLockOpen;
    frame.lockDurationInMilliseconds = pendingLockDuration;
    frame.beep = pendingBeep;
    frame.beepPattern = pendingBeepPattern;

    pendingLockOpen = false;
    pendingLockDuration = 0;
    pendingBeep = false;
    pendingBeepPattern = BuzzerPattern::Off;

    // LED Color logic
    if (!brokerConnected) {
        frame.ledColor = COLOR_RED;
    } else if (doorPhase == DoorPhase::Maintenance) {
        frame.ledColor = COLOR_AMBER;
    } else if (doorPhase == DoorPhase::ShowingDecision) {
        if (doorMode == DoorMode::Reserved) {
            frame.ledColor = reservedColor;
        } else {
            frame.ledColor = lastDecisionGranted ? COLOR_GREEN : COLOR_RED;
        }
    } else { // Idle or WaitingDecision
        if (doorMode == DoorMode::Reserved) {
            frame.ledColor = reservedColor;
        } else {
            frame.ledColor = COLOR_WHITE;
        }
    }

    // Headline logic
    if (doorPhase == DoorPhase::Maintenance) {
        frame.headline = maintenanceMessage;
    } else if (doorPhase == DoorPhase::WaitingDecision || doorPhase == DoorPhase::ShowingDecision) {
        frame.headline = customHeadline;
    } else { // Idle
        if (doorMode == DoorMode::Reserved) {
            frame.headline = "RESERVADO";
        } else {
            frame.headline = nullptr;
        }
    }

    return frame;
}

DoorMode DoorPresentation::mode() const {
    return doorMode;
}

DoorPhase DoorPresentation::phase() const {
    return doorPhase;
}
