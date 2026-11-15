#ifndef DIMMABLE_LED_H
#define DIMMABLE_LED_H

/**
 * @file DimmableLed.h
 * @brief Declares an LED (or NPN-switched LED group) with hardware fades through LEDC.
 * @ingroup actuators_module
 */

#include <ModestIoT.h>
#include <Arduino.h>

/**
 * @name Actuators: Light Output
 * @ingroup actuators_module
 * @{
 */

/**
 * @brief Fades a PWM output in and out with `ledcFade` (the ESP32 LEDC peripheral).
 * @ingroup actuators_module
 *
 * The fade runs in hardware, so the CPU is free meanwhile. A new fade is only started once
 * the previous one has finished: `FADE_IN`/`FADE_OUT` record the wanted state and `update()`
 * (called from the device tick) starts the fade when the channel is free. This never blocks.
 */
class DimmableLed : public Actuator {
private:
    const uint8_t brightness; ///< Duty when on (0-255).
    const uint32_t fadeInInMilliseconds;
    const uint32_t fadeOutInMilliseconds;

    bool wantedOn; ///< State requested by the last command.
    bool shownOn; ///< State of the last fade started.
    unsigned long fadeEndTimestamp; ///< When the running fade finishes.
    unsigned long fadeStartTimestamp; ///< When the running fade started (software fade).
    uint8_t currentDuty; ///< Duty written to the pin right now.
    uint8_t fadeFromDuty; ///< Duty at the start of the running fade.

    static constexpr uint8_t PWM_RESOLUTION_IN_BITS = 8;

protected:
    void executeCommand(Command command) override;

public:
    static constexpr int FADE_IN_COMMAND_IDENTIFIER = 320;
    static constexpr int FADE_OUT_COMMAND_IDENTIFIER = 321;
    static constexpr Command FADE_IN_COMMAND{FADE_IN_COMMAND_IDENTIFIER};
    static constexpr Command FADE_OUT_COMMAND{FADE_OUT_COMMAND_IDENTIFIER};

    /**
     * @brief Attaches the LEDC channel with the output off.
     */
    DimmableLed(int gpioPin, uint8_t brightness, uint32_t fadeInInMilliseconds, uint32_t fadeOutInMilliseconds,
                uint32_t pwmFrequencyInHertz = 5000, CommandHandler* parentHandler = nullptr);

    /** @brief Starts the pending fade if the channel is free. Call it periodically. */
    void update(unsigned long nowInMilliseconds);

    /** @brief State requested by the last command (the fade may still be running). */
    [[nodiscard]] bool isActive() const;
};

/** @} */ // End of Actuators: Light Output

#endif // DIMMABLE_LED_H
