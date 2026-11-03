/**
 * @file DoorPresentation.cpp
 * @brief Implements pure state machine governing door visual/acoustic feedback, roles and timeouts.
 */

#include "DoorPresentation.h"
#include <stdio.h>
#include <string.h>

namespace {
constexpr RgbColor COLOR_WHITE{255, 255, 255};
constexpr RgbColor COLOR_ENROLLMENT{0, 180, 255};   // cyan: distinct from any reservation colour
constexpr uint32_t IDENTIFY_BLINK_IN_MILLISECONDS = 250;
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
      pendingBeepPattern(BuzzerPattern::Off),
      enrollStartedTimestamp(0),
      enrollDurationInMilliseconds(0),
      registered(true),
      identifying(false),
      identifyStartedTimestamp(0),
      identifyDurationInMilliseconds(0)
{
    unregisteredHeadline[0] = '\0';
    customHeadline[0] = '\0';
    maintenanceMessage[0] = '\0';
}

bool DoorPresentation::onCardRead(uint32_t nowInMilliseconds) {
    if (!registered || doorPhase == DoorPhase::WaitingDecision || doorPhase == DoorPhase::Maintenance) {
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
    registered = true;   // only an adopted node receives `mode`
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

bool DoorPresentation::onEnroll(bool active, uint32_t durationInMilliseconds, uint32_t nowInMilliseconds) {
    if (!registered || doorPhase == DoorPhase::Maintenance) {
        return false;
    }
    if (active) {
        doorPhase = DoorPhase::Enrolling;
        enrollStartedTimestamp = nowInMilliseconds;
        enrollDurationInMilliseconds = durationInMilliseconds;
    } else if (doorPhase == DoorPhase::Enrolling) {
        doorPhase = DoorPhase::Idle;
    }
    return true;
}

bool DoorPresentation::onEnrollmentCard(uint32_t nowInMilliseconds) {
    if (doorPhase != DoorPhase::Enrolling) {
        return false;
    }
    // Feedback reuses the decision phase, but the lock never opens
    doorPhase = DoorPhase::ShowingDecision;
    decisionStartedTimestamp = nowInMilliseconds;
    lastDecisionGranted = false;
    pendingLockOpen = false;
    pendingLockDuration = 0;
    pendingBeep = true;
    pendingBeepPattern = BuzzerPattern::Granted;
    strncpy(customHeadline, "Tarjeta capturada", sizeof(customHeadline) - 1);
    customHeadline[sizeof(customHeadline) - 1] = '\0';
    return true;
}

bool DoorPresentation::isEnrolling() const {
    return doorPhase == DoorPhase::Enrolling;
}

void DoorPresentation::requireRegistration(const char* pairingCode) {
    registered = false;
    snprintf(unregisteredHeadline, sizeof(unregisteredHeadline), "Sin registrar\nCodigo %s",
             pairingCode != nullptr ? pairingCode : "----");
}

bool DoorPresentation::isRegistered() const {
    return registered;
}

void DoorPresentation::onIdentify(uint32_t durationInMilliseconds, uint32_t nowInMilliseconds) {
    identifying = true;
    identifyStartedTimestamp = nowInMilliseconds;
    identifyDurationInMilliseconds = durationInMilliseconds;
    pendingBeep = true;
    pendingBeepPattern = BuzzerPattern::Granted;
}

PresentationFrame DoorPresentation::tick(uint32_t nowInMilliseconds) {
    if (identifying && nowInMilliseconds - identifyStartedTimestamp >= identifyDurationInMilliseconds) {
        identifying = false;
    }
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
    } else if (doorPhase == DoorPhase::Enrolling) {
        // The window ends on the node too, even if the cancel never arrives
        if (nowInMilliseconds - enrollStartedTimestamp >= enrollDurationInMilliseconds) {
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

    // LED logic: the RGB LED is lit only while the door is RESERVED (and not in maintenance);
    // in any other state it stays off and the OLED and buzzer carry the feedback
    // Exception: enrollment mode lights it cyan so the admin sees which reader is listening
    // Identify overrides everything: a white blink that is easy to spot among several boards
    if (identifying) {
        frame.ledOn = ((nowInMilliseconds - identifyStartedTimestamp) / IDENTIFY_BLINK_IN_MILLISECONDS) % 2 == 0;
        frame.ledColor = frame.ledOn ? COLOR_WHITE : RgbColor{0, 0, 0};
    } else if (doorPhase == DoorPhase::Enrolling) {
        frame.ledOn = true;
        frame.ledColor = COLOR_ENROLLMENT;
    } else {
        frame.ledOn = doorMode == DoorMode::Reserved && doorPhase != DoorPhase::Maintenance;
        frame.ledColor = frame.ledOn ? reservedColor : RgbColor{0, 0, 0};
    }

    // Headline logic
    if (!registered) {
        frame.headline = unregisteredHeadline;
    } else if (doorPhase == DoorPhase::Maintenance) {
        frame.headline = maintenanceMessage;
    } else if (doorPhase == DoorPhase::Enrolling) {
        frame.headline = "MODO REGISTRO";
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

RgbColor DoorPresentation::reservedLedColor() const {
    return reservedColor;
}
