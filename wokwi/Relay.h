#ifndef RELAY_H
#define RELAY_H

/**
 * @file Relay.h
 * @brief Declares a relay module actuator with configurable trigger polarity.
 * @ingroup actuators_module
 */

#include <ModestIoT.h>

/**
 * @name Actuators: Power Switching
 * @ingroup actuators_module
 * @{
 */

/**
 * @brief Digital output actuator that energizes or releases a relay coil.
 * @ingroup actuators_module
 *
 * Most opto-isolated relay boards trigger on LOW, so the output polarity is configurable.
 * The idle level is written before the pin is switched to output to avoid a click at boot.
 */
class Relay : public Actuator {
private:
    bool energized; ///< Current logical state of the relay coil.
    const bool activeLow; ///< `true` when the board energizes the coil with a LOW level.

    /**
     * @brief Writes the electrical level that matches the logical state.
     */
    void writeOutputLevel();

protected:
    /**
     * @brief Executes relay state commands.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    /** @brief Command identifier for energizing the coil. */
    static constexpr int ENERGIZE_COMMAND_IDENTIFIER = 300;
    /** @brief Command identifier for releasing the coil. */
    static constexpr int RELEASE_COMMAND_IDENTIFIER = 301;

    /** @brief Prebuilt command instance for energizing the coil. */
    static constexpr Command ENERGIZE_COMMAND{ENERGIZE_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for releasing the coil. */
    static constexpr Command RELEASE_COMMAND{RELEASE_COMMAND_IDENTIFIER};

    /**
     * @brief Constructs a relay actuator in the released state.
     * @param signalGpioPin Output GPIO pin connected to the relay IN input.
     * @param activeLow `true` when the board triggers on LOW.
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    Relay(int signalGpioPin, bool activeLow = true, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Reports the current logical relay state.
     * @return `true` if the coil is energized, otherwise `false`.
     */
    [[nodiscard]] bool isEnergized() const;
};

/** @} */ // End of Actuators: Power Switching

#endif // RELAY_H
