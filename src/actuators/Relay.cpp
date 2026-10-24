/**
 * @file Relay.cpp
 * @brief Implements command-driven relay switching.
 */

#include "Relay.h"
#include <Arduino.h>

Relay::Relay(int signalGpioPin, bool activeLow, CommandHandler* parentHandler)
    : Actuator(signalGpioPin, parentHandler), energized(false), activeLow(activeLow) {
    writeOutputLevel();     // sets the idle level before enabling the output
    pinMode(pin, OUTPUT);
    writeOutputLevel();
}

void Relay::executeCommand(Command command) {
    if (command == ENERGIZE_COMMAND) {
        energized = true;
    }
    else if (command == RELEASE_COMMAND) {
        energized = false;
    }
    writeOutputLevel();
}

void Relay::writeOutputLevel() {
    digitalWrite(pin, (energized != activeLow) ? HIGH : LOW);
}

bool Relay::isEnergized() const { return energized; }
