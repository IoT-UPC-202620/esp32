#ifndef RGB_LED_H
#define RGB_LED_H

/**
 * @file RgbLed.h
 * @brief Declares a PWM-driven RGB LED actuator with buffered color updates.
 * @ingroup actuators_module
 */

#include "../ModestIoT/Actuator.h"
#include <Arduino.h>

/**
 * @name Actuators: Color Output
 * @ingroup actuators_module
 * @{
 */

/**
 * @brief Four-leg RGB LED driven by three LEDC PWM outputs.
 * @ingroup actuators_module
 *
 * The base `pin` attribute holds the red pin. Color values are buffered via `setColor()`
 * before the `APPLY_COLOR_COMMAND` is issued, following the `Buzzer::setTone()` pattern.
 */
class RgbLed : public Actuator {
private:
    const int greenPin; ///< PWM pin connected to the green leg.
    const int bluePin; ///< PWM pin connected to the blue leg.
    const bool commonAnode; ///< `true` when the common leg goes to 3V3 (inverted duty).

    uint8_t bufferedRed; ///< Red intensity applied on the next `APPLY_COLOR_COMMAND`.
    uint8_t bufferedGreen; ///< Green intensity applied on the next `APPLY_COLOR_COMMAND`.
    uint8_t bufferedBlue; ///< Blue intensity applied on the next `APPLY_COLOR_COMMAND`.
    bool active; ///< `true` while a color is being shown.

    static constexpr uint8_t PWM_RESOLUTION_IN_BITS = 8; ///< Duty resolution (0-255).

    /**
     * @brief Writes the given intensities to the three PWM outputs.
     */
    void writeChannels(uint8_t red, uint8_t green, uint8_t blue) const;

protected:
    /**
     * @brief Executes color commands.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    /** @brief Command identifier for showing the buffered color. */
    static constexpr int APPLY_COLOR_COMMAND_IDENTIFIER = 310;
    /** @brief Command identifier for turning the LED off. */
    static constexpr int TURN_OFF_COMMAND_IDENTIFIER = 311;

    /** @brief Prebuilt command instance for showing the buffered color. */
    static constexpr Command APPLY_COLOR_COMMAND{APPLY_COLOR_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for turning the LED off. */
    static constexpr Command TURN_OFF_COMMAND{TURN_OFF_COMMAND_IDENTIFIER};

    /**
     * @brief Constructs the actuator and attaches the three LEDC channels (LED off).
     * @param redGpioPin PWM pin for the red leg.
     * @param greenGpioPin PWM pin for the green leg.
     * @param blueGpioPin PWM pin for the blue leg.
     * @param commonAnode `true` for common anode, `false` for common cathode.
     * @param pwmFrequencyInHertz LEDC carrier frequency.
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    RgbLed(int redGpioPin, int greenGpioPin, int blueGpioPin, bool commonAnode = false,
           uint32_t pwmFrequencyInHertz = 5000, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Buffers the color for the next `APPLY_COLOR_COMMAND`.
     */
    void setColor(uint8_t red, uint8_t green, uint8_t blue);

    /** @brief Reports whether a color is currently shown. */
    [[nodiscard]] bool isActive() const;
    [[nodiscard]] uint8_t getRed() const;
    [[nodiscard]] uint8_t getGreen() const;
    [[nodiscard]] uint8_t getBlue() const;
};

/** @} */ // End of Actuators: Color Output

#endif // RGB_LED_H
