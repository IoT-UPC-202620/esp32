/**
 * @file RgbLed.cpp
 * @brief Implements buffered PWM color output for RGB LEDs.
 */

#include "RgbLed.h"

RgbLed::RgbLed(int redGpioPin, int greenGpioPin, int blueGpioPin, bool commonAnode,
               uint32_t pwmFrequencyInHertz, CommandHandler* parentHandler)
    : Actuator(redGpioPin, parentHandler),
      greenPin(greenGpioPin),
      bluePin(blueGpioPin),
      commonAnode(commonAnode),
      bufferedRed(0),
      bufferedGreen(0),
      bufferedBlue(0),
      active(false)
{
    ledcAttach(pin, pwmFrequencyInHertz, PWM_RESOLUTION_IN_BITS);
    ledcAttach(greenPin, pwmFrequencyInHertz, PWM_RESOLUTION_IN_BITS);
    ledcAttach(bluePin, pwmFrequencyInHertz, PWM_RESOLUTION_IN_BITS);
    writeChannels(0, 0, 0);
}

void RgbLed::executeCommand(Command command) {
    if (command == APPLY_COLOR_COMMAND) {
        writeChannels(bufferedRed, bufferedGreen, bufferedBlue);
        active = true;
    }
    else if (command == TURN_OFF_COMMAND) {
        writeChannels(0, 0, 0);
        active = false;
    }
}

void RgbLed::writeChannels(uint8_t red, uint8_t green, uint8_t blue) const {
    if (commonAnode) {
        red = 255 - red; green = 255 - green; blue = 255 - blue;
    }
    ledcWrite(pin, red);
    ledcWrite(greenPin, green);
    ledcWrite(bluePin, blue);
}

void RgbLed::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    bufferedRed = red;
    bufferedGreen = green;
    bufferedBlue = blue;
}

bool RgbLed::isActive() const { return active; }
uint8_t RgbLed::getRed() const { return bufferedRed; }
uint8_t RgbLed::getGreen() const { return bufferedGreen; }
uint8_t RgbLed::getBlue() const { return bufferedBlue; }
