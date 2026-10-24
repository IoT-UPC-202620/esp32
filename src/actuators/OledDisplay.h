#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

/**
 * @file OledDisplay.h
 * @brief Declares an I2C SSD1306 OLED actuator that renders the node status screens.
 * @ingroup actuators_module
 */

#include "../ModestIoT/Actuator.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/**
 * @name Actuators: Display & Graphic Output
 * @ingroup actuators_module
 * @{
 */

/**
 * @brief Actuator adapter for SSD1306 graphic displays over I2C.
 * @ingroup actuators_module
 *
 * Mirrors `CharacterLcdDisplay`: the base `pin` attribute holds the I2C address, screen
 * content is staged through setters, and a command commits one of the screen layouts.
 */
class OledDisplay : public Actuator {
private:
    Adafruit_SSD1306 physicalOledScreen; ///< Underlying Adafruit SSD1306 driver instance.
    const uint8_t screenWidth; ///< Width of the panel in pixels.
    bool ready; ///< `true` when the controller answered during initialization.

    static constexpr size_t TEXT_BUFFER_SIZE = 32; ///< Capacity of each staged text field.
    static constexpr size_t HEADLINE_BUFFER_SIZE = 65; ///< Capacity for headline text.

    char bufferedMessage[TEXT_BUFFER_SIZE]; ///< Text for the message screen.
    char bufferedHeadline[HEADLINE_BUFFER_SIZE]; ///< Text for the headline screen.
    float bufferedTemperatureInCelsius; ///< Temperature for the dashboard screen.
    float bufferedRelativeHumidityInPercentage; ///< Humidity for the dashboard screen.
    bool climateAvailable; ///< `false` until the first climate reading is staged.
    char bufferedLastCardUid[TEXT_BUFFER_SIZE]; ///< Compact UID shown on the dashboard.
    char bufferedCardUid[TEXT_BUFFER_SIZE]; ///< Spaced UID shown on the card screen.
    char bufferedCardType[TEXT_BUFFER_SIZE]; ///< PICC type shown on the card screen.

    void renderMessageScreen(); ///< Draws the centered message screen.
    void renderHeadlineScreen(); ///< Draws the status headline screen.
    void renderDashboardScreen(); ///< Draws temperature, humidity and last card.
    void renderCardScreen(); ///< Draws the detected card UID and type.

protected:
    /**
     * @brief Commits one of the screen layouts with the staged content.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    static constexpr int SHOW_MESSAGE_COMMAND_IDENTIFIER = 320;
    static constexpr int SHOW_DASHBOARD_COMMAND_IDENTIFIER = 321;
    static constexpr int SHOW_CARD_COMMAND_IDENTIFIER = 322;
    static constexpr int CLEAR_COMMAND_IDENTIFIER = 323;
    static constexpr int SHOW_HEADLINE_COMMAND_IDENTIFIER = 324;

    static constexpr Command SHOW_MESSAGE_COMMAND{SHOW_MESSAGE_COMMAND_IDENTIFIER};
    static constexpr Command SHOW_DASHBOARD_COMMAND{SHOW_DASHBOARD_COMMAND_IDENTIFIER};
    static constexpr Command SHOW_CARD_COMMAND{SHOW_CARD_COMMAND_IDENTIFIER};
    static constexpr Command CLEAR_COMMAND{CLEAR_COMMAND_IDENTIFIER};
    static constexpr Command SHOW_HEADLINE_COMMAND{SHOW_HEADLINE_COMMAND_IDENTIFIER};

    /**
     * @brief Constructs the display and initializes the SSD1306 controller.
     * @param i2cAddress Mapped to the base framework 'pin' attribute via casting.
     * @param widthInPixels Panel width (e.g., 128).
     * @param heightInPixels Panel height (e.g., 32, 64).
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    OledDisplay(uint8_t i2cAddress, uint8_t widthInPixels, uint8_t heightInPixels,
                CommandHandler* parentHandler = nullptr);

    /** @brief Stages the text shown by `SHOW_MESSAGE_COMMAND`. */
    void setMessage(const char* nullTerminatedString);
    /** @brief Stages the text shown by `SHOW_HEADLINE_COMMAND`. */
    void setHeadline(const char* nullTerminatedString);
    /** @brief Stages the climate values shown by `SHOW_DASHBOARD_COMMAND`. */
    void setClimate(float temperatureInCelsius, float relativeHumidityInPercentage);
    /** @brief Stages the last card UID shown by `SHOW_DASHBOARD_COMMAND`. */
    void setLastCardUid(const char* compactUid);
    /** @brief Stages the card shown by `SHOW_CARD_COMMAND`. */
    void setDetectedCard(const char* spacedUid, const char* cardType);

    /**
     * @brief Reports whether the controller answered during initialization.
     * @return `true` when the display is usable.
     */
    [[nodiscard]] bool isReady() const;
};

/** @} */ // End of Actuators: Display & Graphic Output

#endif // OLED_DISPLAY_H
