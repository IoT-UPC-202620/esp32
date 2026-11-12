/**
 * @file OledDisplay.cpp
 * @brief Implements the status screens rendered on the SSD1306 OLED.
 */

#include "OledDisplay.h"
#include <Wire.h>

OledDisplay::OledDisplay(uint8_t i2cAddress, uint8_t widthInPixels, uint8_t heightInPixels,
                         CommandHandler* parentHandler)
    : Actuator(static_cast<int>(i2cAddress), parentHandler),
      physicalOledScreen(widthInPixels, heightInPixels, &Wire, -1),
      screenWidth(widthInPixels),
      ready(false),
      bufferedMessage(""),
      bufferedHeadline(""),
      bufferedTemperatureInCelsius(0.0f),
      bufferedRelativeHumidityInPercentage(0.0f),
      climateAvailable(false),
      bufferedDistanceInCentimeters(-1.0f),
      bufferedPresenceLightsOn(false),
      bufferedCardUid(""),
      bufferedCardType("")
{
    ready = physicalOledScreen.begin(SSD1306_SWITCHCAPVCC, i2cAddress);
    if (ready) {
        physicalOledScreen.setTextColor(SSD1306_WHITE);
        physicalOledScreen.clearDisplay();
        physicalOledScreen.display();
    }
}

void OledDisplay::executeCommand(Command command) {
    if (!ready) return;

    if (command == SHOW_MESSAGE_COMMAND) {
        renderMessageScreen();
    }
    else if (command == SHOW_HEADLINE_COMMAND) {
        renderHeadlineScreen();
    }
    else if (command == SHOW_DASHBOARD_COMMAND) {
        renderDashboardScreen();
    }
    else if (command == SHOW_CARD_COMMAND) {
        renderCardScreen();
    }
    else if (command == CLEAR_COMMAND) {
        physicalOledScreen.clearDisplay();
        physicalOledScreen.display();
    }
}

void OledDisplay::renderMessageScreen() {
    physicalOledScreen.clearDisplay();
    physicalOledScreen.setTextSize(1);
    physicalOledScreen.setCursor(0, 28);
    physicalOledScreen.print(bufferedMessage);
    physicalOledScreen.display();
}

void OledDisplay::renderHeadlineScreen() {
    physicalOledScreen.clearDisplay();
    physicalOledScreen.setTextSize(1);
    physicalOledScreen.setCursor(0, 28);
    physicalOledScreen.print(bufferedHeadline);
    physicalOledScreen.display();
}

void OledDisplay::renderDashboardScreen() {
    physicalOledScreen.clearDisplay();

    physicalOledScreen.setTextSize(1);
    physicalOledScreen.setCursor(0, 0);   physicalOledScreen.print("TEMP C");
    physicalOledScreen.setCursor(72, 0);  physicalOledScreen.print("HUM %");

    physicalOledScreen.setTextSize(2);
    physicalOledScreen.setCursor(0, 10);
    if (climateAvailable) physicalOledScreen.print(bufferedTemperatureInCelsius, 1);
    else physicalOledScreen.print("--.-");
    physicalOledScreen.setCursor(72, 10);
    if (climateAvailable) physicalOledScreen.print(bufferedRelativeHumidityInPercentage, 0);
    else physicalOledScreen.print("--");

    physicalOledScreen.drawLine(0, 32, screenWidth - 1, 32, SSD1306_WHITE);

    physicalOledScreen.setTextSize(1);
    physicalOledScreen.setCursor(0, 38);   physicalOledScreen.print("DISTANCIA");
    physicalOledScreen.setCursor(96, 38);  physicalOledScreen.print("LUZ");

    physicalOledScreen.setTextSize(2);
    physicalOledScreen.setCursor(0, 48);
    if (bufferedDistanceInCentimeters > 0.0f) {
        physicalOledScreen.print(bufferedDistanceInCentimeters, 0);
        physicalOledScreen.print("cm");
    } else {
        physicalOledScreen.print("--");   // no echo received yet
    }

    physicalOledScreen.setTextSize(1);
    physicalOledScreen.setCursor(96, 52);
    physicalOledScreen.print(bufferedPresenceLightsOn ? "ON" : "OFF");

    physicalOledScreen.display();
}

void OledDisplay::renderCardScreen() {
    physicalOledScreen.clearDisplay();

    physicalOledScreen.setTextSize(1);
    physicalOledScreen.setCursor(0, 0);
    physicalOledScreen.print("TARJETA DETECTADA");
    physicalOledScreen.drawLine(0, 10, screenWidth - 1, 10, SSD1306_WHITE);

    physicalOledScreen.setCursor(0, 16);
    physicalOledScreen.print("UID:");

    char compactUid[TEXT_BUFFER_SIZE];
    size_t compactLength = 0;
    for (const char* character = bufferedCardUid; *character != '\0'; ++character) {
        if (*character != ' ') compactUid[compactLength++] = *character;
    }
    compactUid[compactLength] = '\0';

    if (compactLength <= 10) {   // 4-byte UID: 8 large characters fit in 128 px
        physicalOledScreen.setTextSize(2);
        physicalOledScreen.setCursor(0, 28);
        physicalOledScreen.print(compactUid);
    } else {                     // 7-byte UID: small font with separators
        physicalOledScreen.setTextSize(1);
        physicalOledScreen.setCursor(0, 32);
        physicalOledScreen.print(bufferedCardUid);
    }

    physicalOledScreen.setTextSize(1);
    physicalOledScreen.setCursor(0, 56);
    physicalOledScreen.print(bufferedCardType);

    physicalOledScreen.display();
}

void OledDisplay::setMessage(const char* nullTerminatedString) {
    if (nullTerminatedString == nullptr) return;
    snprintf(bufferedMessage, TEXT_BUFFER_SIZE, "%s", nullTerminatedString);
}

void OledDisplay::setHeadline(const char* nullTerminatedString) {
    if (nullTerminatedString == nullptr) return;
    snprintf(bufferedHeadline, HEADLINE_BUFFER_SIZE, "%s", nullTerminatedString);
}

void OledDisplay::setClimate(float temperatureInCelsius, float relativeHumidityInPercentage) {
    bufferedTemperatureInCelsius = temperatureInCelsius;
    bufferedRelativeHumidityInPercentage = relativeHumidityInPercentage;
    climateAvailable = true;
}

void OledDisplay::setPresence(float distanceInCentimeters, bool presenceLightsOn) {
    bufferedDistanceInCentimeters = distanceInCentimeters;
    bufferedPresenceLightsOn = presenceLightsOn;
}

void OledDisplay::setDetectedCard(const char* spacedUid, const char* cardType) {
    if (spacedUid == nullptr || cardType == nullptr) return;
    snprintf(bufferedCardUid, TEXT_BUFFER_SIZE, "%s", spacedUid);
    snprintf(bufferedCardType, TEXT_BUFFER_SIZE, "%s", cardType);
}

bool OledDisplay::isReady() const { return ready; }
