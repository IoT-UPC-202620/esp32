#ifndef LCD_STATUS_DISPLAY_H
#define LCD_STATUS_DISPLAY_H

/**
 * @file LcdStatusDisplay.h
 * @brief Pantalla de estado del nodo sobre la LCD de caracteres de ModestIoT.
 *
 * Reemplaza a OledDisplay en Wokwi con la misma interfaz (setMessage, setHeadline,
 * setClimate, setPresence, setDetectedCard y los comandos SHOW_*). Compone un
 * `CharacterLcdDisplay` y solo escribe por I2C cuando el contenido cambia.
 */

#include <ModestIoT.h>

class LcdStatusDisplay : public Actuator {
private:
    static constexpr uint8_t MAX_COLUMNS = 20;
    static constexpr uint8_t MAX_ROWS = 4;
    static constexpr size_t TEXT_BUFFER_SIZE = 65;

    CharacterLcdDisplay characterDisplay; ///< Driver de ModestIoT.
    const uint8_t columns;
    const uint8_t rows;
    bool ready;

    char bufferedMessage[TEXT_BUFFER_SIZE];
    char bufferedHeadline[TEXT_BUFFER_SIZE];
    char bufferedCardUid[24];
    char bufferedCardType[24];
    float bufferedTemperatureInCelsius;
    float bufferedRelativeHumidityInPercentage;
    bool climateAvailable;
    float bufferedDistanceInCentimeters;
    bool bufferedPresenceLightsOn;

    char pendingLines[MAX_ROWS][MAX_COLUMNS + 1];   ///< Pantalla a mostrar.
    char shownLines[MAX_ROWS][MAX_COLUMNS + 1];     ///< Lo ultimo escrito en la LCD.

    void clearPendingLines();
    void writeWrapped(const char* text, uint8_t firstRow, uint8_t lastRow);
    void writeClimateLine(uint8_t row);
    void commit();

    void renderText(const char* text);
    void renderDashboard();
    void renderCard();

protected:
    void executeCommand(Command command) override;

public:
    static constexpr int SHOW_MESSAGE_COMMAND_IDENTIFIER = 330;
    static constexpr int SHOW_DASHBOARD_COMMAND_IDENTIFIER = 331;
    static constexpr int SHOW_CARD_COMMAND_IDENTIFIER = 332;
    static constexpr int CLEAR_COMMAND_IDENTIFIER = 333;
    static constexpr int SHOW_HEADLINE_COMMAND_IDENTIFIER = 334;
    static constexpr Command SHOW_MESSAGE_COMMAND{SHOW_MESSAGE_COMMAND_IDENTIFIER};
    static constexpr Command SHOW_DASHBOARD_COMMAND{SHOW_DASHBOARD_COMMAND_IDENTIFIER};
    static constexpr Command SHOW_CARD_COMMAND{SHOW_CARD_COMMAND_IDENTIFIER};
    static constexpr Command CLEAR_COMMAND{CLEAR_COMMAND_IDENTIFIER};
    static constexpr Command SHOW_HEADLINE_COMMAND{SHOW_HEADLINE_COMMAND_IDENTIFIER};

    LcdStatusDisplay(uint8_t i2cAddress, uint8_t columnCount, uint8_t rowCount,
                     CommandHandler* parentHandler = nullptr);

    void setMessage(const char* text);
    void setHeadline(const char* text);
    void setClimate(float temperatureInCelsius, float relativeHumidityInPercentage);
    void setPresence(float distanceInCentimeters, bool presenceLightsOn);
    void setDetectedCard(const char* uid, const char* cardType);

    [[nodiscard]] bool isReady() const;
};

#endif // LCD_STATUS_DISPLAY_H
