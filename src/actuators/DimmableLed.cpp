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
      fadeEndTimestamp(0)
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

void DimmableLed::update(unsigned long nowInMilliseconds) {
    if (wantedOn == shownOn) return;
    // Wait for the running fade: starting another one on a busy channel can block
    if (static_cast<long>(nowInMilliseconds - fadeEndTimestamp) < 0) return;

    uint32_t from = shownOn ? brightness : 0;
    uint32_t to = wantedOn ? brightness : 0;
    uint32_t duration = wantedOn ? fadeInInMilliseconds : fadeOutInMilliseconds;
    if (!ledcFade(pin, from, to, static_cast<int>(duration))) {
        ledcWrite(pin, to);   // fade unavailable: switch at once
        duration = 0;
    }
    shownOn = wantedOn;
    fadeEndTimestamp = nowInMilliseconds + duration;
}

bool DimmableLed::isActive() const { return wantedOn; }
