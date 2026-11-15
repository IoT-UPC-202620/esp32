/**
 * @file LcdStatusDisplay.cpp
 * @brief Implementa las pantallas del nodo sobre CharacterLcdDisplay (LCD 20x4).
 */

#include "LcdStatusDisplay.h"
#include <Wire.h>
#include <stdio.h>
#include <string.h>

LcdStatusDisplay::LcdStatusDisplay(uint8_t i2cAddress, uint8_t columnCount, uint8_t rowCount,
                                   CommandHandler* parentHandler)
    : Actuator(static_cast<int>(i2cAddress), parentHandler),
      characterDisplay(i2cAddress, columnCount, rowCount),   // init() de la LCD (arranca Wire)
      columns(columnCount < MAX_COLUMNS ? columnCount : MAX_COLUMNS),
      rows(rowCount < MAX_ROWS ? rowCount : MAX_ROWS),
      ready(false),
      bufferedMessage(""),
      bufferedHeadline(""),
      bufferedCardUid(""),
      bufferedCardType(""),
      bufferedTemperatureInCelsius(0.0f),
      bufferedRelativeHumidityInPercentage(0.0f),
      climateAvailable(false),
      bufferedDistanceInCentimeters(-1.0f),
      bufferedPresenceLightsOn(false)
{
    Wire.beginTransmission(i2cAddress);
    ready = (Wire.endTransmission() == 0);
    clearPendingLines();
    for (uint8_t row = 0; row < MAX_ROWS; ++row) shownLines[row][0] = '\0';
}

void LcdStatusDisplay::executeCommand(Command command) {
    if (!ready) return;
    if (command == SHOW_MESSAGE_COMMAND) {
        renderText(bufferedMessage);
    } else if (command == SHOW_HEADLINE_COMMAND) {
        renderText(bufferedHeadline);
    } else if (command == SHOW_DASHBOARD_COMMAND) {
        renderDashboard();
    } else if (command == SHOW_CARD_COMMAND) {
        renderCard();
    } else if (command == CLEAR_COMMAND) {
        clearPendingLines();
        commit();
    }
}

// ---------------- composicion ----------------
void LcdStatusDisplay::clearPendingLines() {
    for (uint8_t row = 0; row < MAX_ROWS; ++row) {
        memset(pendingLines[row], ' ', columns);
        pendingLines[row][columns] = '\0';
    }
}

// Reparte el texto en filas de `columns` caracteres cortando por palabras; '\n' fuerza salto
void LcdStatusDisplay::writeWrapped(const char* text, uint8_t firstRow, uint8_t lastRow) {
    uint8_t row = firstRow;
    uint8_t column = 0;
    const char* cursor = text;
    while (*cursor != '\0' && row <= lastRow) {
        if (*cursor == '\n') {
            row++;
            column = 0;
            cursor++;
            continue;
        }
        if (*cursor == ' ' && column == 0) {   // sin espacios al inicio de una fila
            cursor++;
            continue;
        }
        size_t wordLength = strcspn(cursor, " \n");
        if (wordLength == 0) {   // un espacio entre palabras
            if (column < columns) pendingLines[row][column++] = ' ';
            cursor++;
            continue;
        }
        if (column > 0 && column + wordLength > columns) {   // la palabra no cabe: fila siguiente
            row++;
            column = 0;
            continue;
        }
        for (size_t i = 0; i < wordLength && row <= lastRow; ++i) {
            if (column >= columns) {   // palabra mas larga que la fila
                row++;
                column = 0;
                if (row > lastRow) break;
            }
            pendingLines[row][column++] = cursor[i];
        }
        cursor += wordLength;
    }
}

void LcdStatusDisplay::writeClimateLine(uint8_t row) {
    char line[MAX_COLUMNS + 1];
    if (climateAvailable) {
        snprintf(line, sizeof(line), "T %.1fC   H %.0f%%", bufferedTemperatureInCelsius,
                 bufferedRelativeHumidityInPercentage);
    } else {
        snprintf(line, sizeof(line), "T --.-C   H --%%");
    }
    size_t length = strlen(line);
    memcpy(pendingLines[row], line, length < columns ? length : columns);
}

// Solo escribe por I2C si alguna fila cambio (la LCD es lenta y parpadea al reescribirla)
void LcdStatusDisplay::commit() {
    bool changed = false;
    for (uint8_t row = 0; row < rows; ++row) {
        if (strcmp(pendingLines[row], shownLines[row]) != 0) {
            changed = true;
            break;
        }
    }
    if (!changed) return;
    for (uint8_t row = 0; row < rows; ++row) {
        characterDisplay.setLineBuffer(row, pendingLines[row]);
        memcpy(shownLines[row], pendingLines[row], sizeof(shownLines[row]));
    }
    characterDisplay.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
}

// ---------------- pantallas ----------------
// Texto (titular o mensaje) en las 3 primeras filas y el clima abajo
void LcdStatusDisplay::renderText(const char* text) {
    clearPendingLines();
    writeWrapped(text, 0, rows - 2);
    writeClimateLine(rows - 1);
    commit();
}

void LcdStatusDisplay::renderDashboard() {
    clearPendingLines();
    char line[MAX_COLUMNS + 1];

    if (climateAvailable) snprintf(line, sizeof(line), "Temp: %.1f C", bufferedTemperatureInCelsius);
    else snprintf(line, sizeof(line), "Temp: --.- C");
    memcpy(pendingLines[0], line, strlen(line));

    if (climateAvailable) snprintf(line, sizeof(line), "Hum : %.0f %%", bufferedRelativeHumidityInPercentage);
    else snprintf(line, sizeof(line), "Hum : -- %%");
    memcpy(pendingLines[1], line, strlen(line));

    if (bufferedDistanceInCentimeters > 0.0f) snprintf(line, sizeof(line), "Dist: %.0f cm", bufferedDistanceInCentimeters);
    else snprintf(line, sizeof(line), "Dist: --");
    memcpy(pendingLines[2], line, strlen(line));

    snprintf(line, sizeof(line), "Luz : %s", bufferedPresenceLightsOn ? "ON" : "OFF");
    memcpy(pendingLines[3], line, strlen(line));
    commit();
}

void LcdStatusDisplay::renderCard() {
    clearPendingLines();
    writeWrapped("TARJETA DETECTADA", 0, 0);
    char line[MAX_COLUMNS + 1];
    snprintf(line, sizeof(line), "UID: %s", bufferedCardUid);
    memcpy(pendingLines[1], line, strlen(line));
    writeWrapped(bufferedCardType, 2, 2);
    writeClimateLine(rows - 1);
    commit();
}

// ---------------- datos ----------------
void LcdStatusDisplay::setMessage(const char* text) {
    if (text != nullptr) snprintf(bufferedMessage, sizeof(bufferedMessage), "%s", text);
}

void LcdStatusDisplay::setHeadline(const char* text) {
    if (text != nullptr) snprintf(bufferedHeadline, sizeof(bufferedHeadline), "%s", text);
}

void LcdStatusDisplay::setClimate(float temperatureInCelsius, float relativeHumidityInPercentage) {
    bufferedTemperatureInCelsius = temperatureInCelsius;
    bufferedRelativeHumidityInPercentage = relativeHumidityInPercentage;
    climateAvailable = true;
}

void LcdStatusDisplay::setPresence(float distanceInCentimeters, bool presenceLightsOn) {
    bufferedDistanceInCentimeters = distanceInCentimeters;
    bufferedPresenceLightsOn = presenceLightsOn;
}

void LcdStatusDisplay::setDetectedCard(const char* uid, const char* cardType) {
    if (uid != nullptr) snprintf(bufferedCardUid, sizeof(bufferedCardUid), "%s", uid);
    if (cardType != nullptr) snprintf(bufferedCardType, sizeof(bufferedCardType), "%s", cardType);
}

bool LcdStatusDisplay::isReady() const { return ready; }
