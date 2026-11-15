/**
 * @file DimmableLed.cpp
 * @brief Implements hardware LEDC fades for the presence lights.
 */

#include "DimmableLed.h"

DimmableLed::DimmableLed(int gpioPin, uint8_t brightness, uint32_t fadeInInMilliseconds, uint32_t fadeOutInMilliseconds,
                         uint32_t pwmFrequencyInHertz, CommandHandler* parentHandler)
    : Actuator(gpioPin, parentHandler),
      brightness(brightness),
      fadeInInMilliseconds(fadeInInMilliseconds),
      fadeOutInMilliseconds(fadeOutInMilliseconds),
      wantedOn(false),
      shownOn(false),
      fadeEndTimestamp(0),
      fadeStartTimestamp(0),
      currentDuty(0),
      fadeFromDuty(0)
{
    ledcAttach(pin, pwmFrequencyInHertz, PWM_RESOLUTION_IN_BITS);
    ledcWrite(pin, 0);
}

void DimmableLed::executeCommand(Command command) {
    if (command == FADE_IN_COMMAND) {
        wantedOn = true;
    } else if (command == FADE_OUT_COMMAND) {
        wantedOn = false;
    }
}

// Wokwi: ledcFade() por hardware se queda bloqueado en el simulador, asi que el fundido
// se hace por software, interpolando el duty en cada llamada (el tick del nodo, ~60 ms)
void DimmableLed::update(unsigned long nowInMilliseconds) {
    uint8_t target = wantedOn ? brightness : 0;
    if (wantedOn != shownOn) {   // empieza un fundido nuevo desde el duty actual
        shownOn = wantedOn;
        fadeFromDuty = currentDuty;
        fadeStartTimestamp = nowInMilliseconds;
        fadeEndTimestamp = nowInMilliseconds + (wantedOn ? fadeInInMilliseconds : fadeOutInMilliseconds);
    }
    if (currentDuty == target) return;

    unsigned long total = fadeEndTimestamp - fadeStartTimestamp;
    unsigned long elapsed = nowInMilliseconds - fadeStartTimestamp;
    uint8_t duty = target;
    if (total > 0 && elapsed < total) {
        long delta = static_cast<long>(target) - static_cast<long>(fadeFromDuty);
        duty = static_cast<uint8_t>(fadeFromDuty + delta * static_cast<long>(elapsed) / static_cast<long>(total));
    }
    if (duty != currentDuty) {
        currentDuty = duty;
        ledcWrite(pin, duty);
    }
}

bool DimmableLed::isActive() const { return wantedOn; }
