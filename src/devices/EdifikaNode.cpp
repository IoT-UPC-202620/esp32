/**
 * @file EdifikaNode.cpp
 * @brief Implements the event/command orchestration of the Edifika node.
 */

#include "EdifikaNode.h"

using namespace NodeConfig;

EdifikaNode::EdifikaNode()
    : Device(SAMPLING_INTERVAL_IN_MILLISECONDS),
      doorLockRelay(LOCK_RELAY_PIN, LOCK_RELAY_ACTIVE_LOW),
      buzzer(BUZZER_PIN),
      statusRgbLed(RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN, RGB_COMMON_ANODE, RGB_PWM_FREQUENCY_IN_HERTZ),
      matrixRowLeds{Led(MATRIX_ROW_PINS[0]), Led(MATRIX_ROW_PINS[1]), Led(MATRIX_ROW_PINS[2])},
      climateSensor(DHT_PIN, DHT_MODEL, this),
      accessCardReader(RFID_SS_PIN, RFID_RESET_PIN, RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, this),
      statusDisplay(OLED_I2C_ADDRESS, OLED_WIDTH, OLED_HEIGHT),
      lockOpenedTimestamp(0),
      cardScreenTimestamp(0),
      lastRenderTimestamp(0),
      cardScreenVisible(false),
      climateAvailable(false),
      lastRgbPhase(-1)
{
}

void EdifikaNode::begin() {
    statusRgbLed.setColor(255, 0, 0);
    statusRgbLed.handle(RgbLed::APPLY_COLOR_COMMAND);

    // Startup beep: if it does not sound here, the problem is the buzzer/wiring, not the RFID
    Serial.println("Probando buzzer...");
    beep(STARTUP_BEEP_DURATION_IN_MILLISECONDS);

    if (accessCardReader.isConnected()) {
        Serial.printf("RC522 listo (version 0x%02X)\n", accessCardReader.getFirmwareVersion());
    } else {
        Serial.println("No encuentro el RC522. Revisa cableado y que este a 3.3V");
    }

    if (!statusDisplay.isReady()) {
        Serial.println("No encuentro la OLED");
        while (true) delay(1000);
    }
    statusDisplay.setMessage("Iniciando...");
    statusDisplay.handle(OledDisplay::SHOW_MESSAGE_COMMAND);
    delay(SPLASH_DURATION_IN_MILLISECONDS);

    appendSensorToScheduler(&accessCardReader, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    appendSensorToScheduler(this, DEVICE_TICK_EVENT_IDENTIFIER);
    appendSensorToScheduler(&climateSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    initializeAsynchronousEngine();
}

void EdifikaNode::on(Event event) {
    switch (event.identifier) {
        case RfidReader::CARD_DETECTED_EVENT_IDENTIFIER:
            onCardDetected();
            break;
        case Sensor::DATA_READ_EVENT_IDENTIFIER:   // only the DHT emits DATA_READ in this node
            onClimateRead();
            break;
        case DEVICE_TICK_EVENT_IDENTIFIER:
            onDeviceTick();
            break;
        default:
            break;
    }
}

void EdifikaNode::handle(Command command) {
    if (command == UNLOCK_DOOR_COMMAND) {
        unlockDoor();
    }
}

// ---------------- event reactions ----------------
void EdifikaNode::onCardDetected() {
    Serial.printf("Tarjeta: %s  (%s)\n", accessCardReader.getUid(), accessCardReader.getCardType());

    beep(BEEP_DURATION_IN_MILLISECONDS);
    handle(UNLOCK_DOOR_COMMAND);       // for now any card opens the lock

    statusDisplay.setLastCardUid(accessCardReader.getCompactUid());
    statusDisplay.setDetectedCard(accessCardReader.getUid(), accessCardReader.getCardType());
    cardScreenVisible = true;
    cardScreenTimestamp = millis();
    lastRenderTimestamp = cardScreenTimestamp;
    statusDisplay.handle(OledDisplay::SHOW_CARD_COMMAND);
}

void EdifikaNode::onClimateRead() {
    climateAvailable = true;
    float temperature = climateSensor.getTemperatureInCelsius();
    float humidity = climateSensor.getRelativeHumidityInPercentage();
    statusDisplay.setClimate(temperature, humidity);
    Serial.printf("T=%.1f C  H=%.0f %%\n", temperature, humidity);
}

void EdifikaNode::onDeviceTick() {
    updateDoorLock();
    updateRgbGradient();
    updateMatrixRows();
    updateDisplay();
}

// ---------------- lock ----------------
// Energizes the lock LOCK_OPEN_DURATION_IN_MILLISECONDS without blocking the rest of the node
void EdifikaNode::unlockDoor() {
    lockOpenedTimestamp = millis();
    doorLockRelay.handle(Relay::ENERGIZE_COMMAND);
    Serial.println("Cerradura abierta");
}

void EdifikaNode::updateDoorLock() {
    if (doorLockRelay.isEnergized() && millis() - lockOpenedTimestamp >= LOCK_OPEN_DURATION_IN_MILLISECONDS) {
        doorLockRelay.handle(Relay::RELEASE_COMMAND);
        Serial.println("Cerradura cerrada");
    }
}

// ---------------- buzzer ----------------
// tone() with a duration stops by itself, so the beep does not block
void EdifikaNode::beep(unsigned long durationInMilliseconds) {
    Serial.println("Beep");
    buzzer.setTone(BUZZER_FREQUENCY_IN_HERTZ, durationInMilliseconds);
    buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
}

// ---------------- rgb led ----------------
// Red -> yellow -> green -> cyan -> blue and back, one phase step every RGB_STEP_IN_MILLISECONDS
void EdifikaNode::updateRgbGradient() {
    constexpr unsigned long PHASE_PEAK = 510;
    unsigned long cycleStep = (millis() / RGB_STEP_IN_MILLISECONDS) % (2 * PHASE_PEAK);
    int phase = static_cast<int>(cycleStep <= PHASE_PEAK ? cycleStep : 2 * PHASE_PEAK - cycleStep);
    if (phase == lastRgbPhase) return;
    lastRgbPhase = phase;

    if (phase <= 255) statusRgbLed.setColor(255 - phase, phase, 0);          // red -> green
    else              statusRgbLed.setColor(0, 510 - phase, phase - 255);    // green -> blue
    statusRgbLed.handle(RgbLed::APPLY_COLOR_COMMAND);
}

// ---------------- 3x3 matrix ----------------
void EdifikaNode::updateMatrixRows() {
    bool hot = climateAvailable && climateSensor.getTemperatureInCelsius() >= HIGH_TEMPERATURE_IN_CELSIUS;
    bool humid = climateAvailable && climateSensor.getRelativeHumidityInPercentage() >= HIGH_HUMIDITY_IN_PERCENTAGE;

    switchLed(matrixRowLeds[0], doorLockRelay.isEnergized());   // row 1: lock open
    switchLed(matrixRowLeds[1], hot);                           // row 2: high temperature
    switchLed(matrixRowLeds[2], humid);                         // row 3: high humidity
}

void EdifikaNode::switchLed(Led& led, bool turnOn) {
    if (led.isActive() != turnOn) {
        led.handle(turnOn ? Led::TURN_ON_COMMAND : Led::TURN_OFF_COMMAND);
    }
}

// ---------------- display ----------------
void EdifikaNode::updateDisplay() {
    unsigned long now = millis();
    if (cardScreenVisible && now - cardScreenTimestamp >= CARD_SCREEN_DURATION_IN_MILLISECONDS) {
        cardScreenVisible = false;
    }

    if (now - lastRenderTimestamp >= DISPLAY_REFRESH_IN_MILLISECONDS) {
        lastRenderTimestamp = now;
        statusDisplay.handle(cardScreenVisible ? OledDisplay::SHOW_CARD_COMMAND : OledDisplay::SHOW_DASHBOARD_COMMAND);
    }
}
