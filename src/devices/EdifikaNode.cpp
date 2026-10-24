/**
 * @file EdifikaNode.cpp
 * @brief Implements the event/command orchestration of the Edifika node.
 */

#include "EdifikaNode.h"
#include "../config/Secrets.h"
#include <time.h>
#include <sys/time.h>

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
      wifiDriver(Secrets::WIFI_SSID, Secrets::WIFI_PASSWORD),
      edgeGateway(wifiDriver, wifiDriver.getNetworkSocket(), Secrets::MQTT_BROKER_HOST, Secrets::MQTT_BROKER_PORT, DEVICE_ID),
      doorPresentation(ACCESS_RESULT_TIMEOUT_IN_MILLISECONDS, DECISION_FEEDBACK_IN_MILLISECONDS),
      lockOpenedTimestamp(0),
      lockOpenDurationInMilliseconds(LOCK_OPEN_DURATION_IN_MILLISECONDS),
      lastHeartbeatTimestamp(0),
      brokerSessionWasActive(false),
      lastReadingsTimestamp(0),
      lastRenderTimestamp(0),
      temporaryMessageTimestamp(0),
      temporaryMessageActive(false),
      climateAvailable(false)
{
    temporaryMessageText[0] = '\0';
}

void EdifikaNode::begin() {
    statusRgbLed.handle(RgbLed::TURN_OFF_COMMAND);   // the RGB LED is only lit while the door is RESERVED

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

    Serial.printf("Conectando a Wi-Fi \"%s\"...\n", Secrets::WIFI_SSID);
    wifiDriver.connect();                 // asynchronous; maintainNetwork() follows it up
    configTime(0, 0, NTP_SERVER);         // UTC clock for the "ts" field once Wi-Fi is up
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

void EdifikaNode::maintainNetwork() {
    edgeGateway.maintainSession();
}

void EdifikaNode::processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const {
    const auto* nodeMessage = static_cast<const NodeMessage*>(rawQueueItemPayload);
    if (!edgeGateway.publish(*nodeMessage)) {
        Serial.printf("Sin publicar (%s): sin conexion con el broker\n", nodeMessage->getChannel());
    }
}

void EdifikaNode::enqueueNodeMessage(NodeMessage* nodeMessage) const {
    TelemetryPackage* telemetryPackage = nodeMessage;
    if (!enqueueTelemetryPayload(&telemetryPackage)) {
        delete telemetryPackage;   // queue full: the network worker is behind, drop it
    }
}

// ---------------- event reactions ----------------
void EdifikaNode::onCardDetected() {
    Serial.printf("Tarjeta: %s  (%s)\n", accessCardReader.getUid(), accessCardReader.getCardType());

    if (!doorPresentation.onCardRead(millis())) {
        return;
    }

    beep(150);
    enqueueNodeMessage(new AccessAttemptMessage(accessCardReader.getCompactUid()));
    if (UNLOCK_ON_ANY_CARD) {
        unlockDoor(LOCK_OPEN_DURATION_IN_MILLISECONDS);
    }

    statusDisplay.setLastCardUid(accessCardReader.getCompactUid());
    statusDisplay.setDetectedCard(accessCardReader.getUid(), accessCardReader.getCardType());
    statusDisplay.setHeadline("Validando...");
    statusDisplay.handle(OledDisplay::SHOW_HEADLINE_COMMAND);
}

void EdifikaNode::onClimateRead() {
    climateAvailable = true;
    float temperature = climateSensor.getTemperatureInCelsius();
    float humidity = climateSensor.getRelativeHumidityInPercentage();
    statusDisplay.setClimate(temperature, humidity);
    Serial.printf("T=%.1f C  H=%.0f %%\n", temperature, humidity);
}

void EdifikaNode::onDeviceTick() {
    doorPresentation.onBrokerSession(edgeGateway.isSessionActive());
    processRemoteCommands();
    updateHeartbeat();
    updateReadings();
    updateDoorLock();

    PresentationFrame frame = doorPresentation.tick(millis());

    if (frame.ledOn) {
        statusRgbLed.setColor(frame.ledColor.red, frame.ledColor.green, frame.ledColor.blue);
        statusRgbLed.handle(RgbLed::APPLY_COLOR_COMMAND);
    } else if (statusRgbLed.isActive()) {
        statusRgbLed.handle(RgbLed::TURN_OFF_COMMAND);   // off outside RESERVED
    }

    if (frame.lockShouldOpen) {
        unlockDoor(frame.lockDurationInMilliseconds > 0 ? frame.lockDurationInMilliseconds : LOCK_OPEN_DURATION_IN_MILLISECONDS);
    }

    if (frame.beep) {
        playBuzzerPattern(frame.beepPattern);
    }

    updateMatrixRows();
    updateDisplay(frame);
}

// ---------------- network ----------------
// Commands arrive on the loop task and are executed here, on the hardware worker,
// so actuators are only ever touched from one task
void EdifikaNode::processRemoteCommands() {
    NodeCommand cmd;
    while (edgeGateway.receiveRemoteCommand(cmd)) {
        bool hasCommandId = (cmd.commandId[0] != '\0');
        const char* ackStatus = "OK";
        const char* ackDetail = "";

        switch (cmd.type) {
            case NodeCommandType::AccessResult:
                doorPresentation.onAccessResult(cmd, millis());
                break;

            case NodeCommandType::Mode:
                doorPresentation.onMode(cmd.mode, cmd.hasColor, cmd.color);
                break;

            case NodeCommandType::Rgb:
                if (!doorPresentation.onRgb(cmd.color)) {
                    ackStatus = "FAILED";
                    ackDetail = "not reserved";
                }
                break;

            case NodeCommandType::Maintenance:
                doorPresentation.onMaintenance(cmd.active, cmd.message);
                break;

            case NodeCommandType::Unlock:
                unlockDoor(cmd.durationInMilliseconds > 0 ? cmd.durationInMilliseconds : LOCK_OPEN_DURATION_IN_MILLISECONDS);
                break;

            case NodeCommandType::Display:
                strncpy(temporaryMessageText, cmd.message, sizeof(temporaryMessageText) - 1);
                temporaryMessageText[sizeof(temporaryMessageText) - 1] = '\0';
                temporaryMessageTimestamp = millis();
                temporaryMessageActive = true;
                statusDisplay.setMessage(temporaryMessageText);
                statusDisplay.handle(OledDisplay::SHOW_MESSAGE_COMMAND);
                break;

            case NodeCommandType::Alert:
                if (cmd.message[0] != '\0') {
                    strncpy(temporaryMessageText, cmd.message, sizeof(temporaryMessageText) - 1);
                    temporaryMessageText[sizeof(temporaryMessageText) - 1] = '\0';
                    temporaryMessageTimestamp = millis();
                    temporaryMessageActive = true;
                    statusDisplay.setMessage(temporaryMessageText);
                    statusDisplay.handle(OledDisplay::SHOW_MESSAGE_COMMAND);
                }
                if (cmd.active) {
                    playBuzzerPattern(cmd.buzzerPattern);
                }
                break;

            case NodeCommandType::Buzzer:
                playBuzzerPattern(cmd.buzzerPattern);
                break;

            case NodeCommandType::TimeSync:
                if (cmd.epochSeconds > 0) {
                    struct timeval tv;
                    tv.tv_sec = static_cast<time_t>(cmd.epochSeconds);
                    tv.tv_usec = 0;
                    settimeofday(&tv, nullptr);
                }
                break;

            case NodeCommandType::Unsupported:
                ackStatus = "FAILED";
                ackDetail = "unsupported command";
                break;

            case NodeCommandType::Invalid:
                ackStatus = "FAILED";
                ackDetail = cmd.error;
                break;
        }

        if (hasCommandId) {
            enqueueNodeMessage(new CommandAckMessage(cmd.commandId, ackStatus, ackDetail));
        }
    }
}

void EdifikaNode::updateHeartbeat() {
    bool sessionActive = edgeGateway.isSessionActive();
    bool sessionJustStarted = sessionActive && !brokerSessionWasActive;
    brokerSessionWasActive = sessionActive;
    if (!sessionActive) return;

    // A new broker session (boot or reconnection) reports its state at once, so the
    // gateway can re-send the latest door mode without waiting for the next period
    // isSessionActive() also reads false while the session mutex is busy, so an early
    // heartbeat is still spaced by MIN_HEARTBEAT_GAP_IN_MILLISECONDS
    unsigned long now = millis();
    unsigned long sinceLast = now - lastHeartbeatTimestamp;
    bool dueByPeriod = lastHeartbeatTimestamp == 0 || sinceLast >= HEARTBEAT_INTERVAL_IN_MILLISECONDS;
    bool dueBySession = sessionJustStarted && sinceLast >= MIN_HEARTBEAT_GAP_IN_MILLISECONDS;
    if (!dueByPeriod && !dueBySession) return;
    lastHeartbeatTimestamp = now;

    bool reserved = doorPresentation.mode() == DoorMode::Reserved;
    char ledColor[8] = "";
    if (reserved) CommandParser::formatHexColor(doorPresentation.reservedLedColor(), ledColor);
    enqueueNodeMessage(new HeartbeatMessage(DEVICE_ID, FIRMWARE_VERSION,
                                            reserved ? "RESERVED" : "FREE_ACCESS", ledColor));
}

void EdifikaNode::updateReadings() {
    if (!climateAvailable || !edgeGateway.isSessionActive()) return;
    unsigned long now = millis();
    if (lastReadingsTimestamp != 0 && now - lastReadingsTimestamp < READINGS_INTERVAL_IN_MILLISECONDS) return;
    lastReadingsTimestamp = now;
    float temperature = climateSensor.getTemperatureInCelsius();
    float humidity = climateSensor.getRelativeHumidityInPercentage();
    enqueueNodeMessage(new SensorReadingMessage("temperature", temperature, "°C"));
    enqueueNodeMessage(new SensorReadingMessage("humidity", humidity, "%"));
}

// ---------------- lock ----------------
// Energizes the lock lockOpenDurationInMilliseconds without blocking the rest of the node
void EdifikaNode::unlockDoor(unsigned long durationInMilliseconds) {
    lockOpenDurationInMilliseconds = durationInMilliseconds > 0 ? durationInMilliseconds : LOCK_OPEN_DURATION_IN_MILLISECONDS;
    lockOpenedTimestamp = millis();
    doorLockRelay.handle(Relay::ENERGIZE_COMMAND);
    Serial.println("Cerradura abierta");
}

void EdifikaNode::updateDoorLock() {
    if (doorLockRelay.isEnergized() && millis() - lockOpenedTimestamp >= lockOpenDurationInMilliseconds) {
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

void EdifikaNode::playBuzzerPattern(BuzzerPattern pattern) {
    switch (pattern) {
        case BuzzerPattern::Granted:
            beep(150);
            break;
        case BuzzerPattern::Denied:
            beep(150);
            delay(200);
            beep(150);
            break;
        case BuzzerPattern::Alert:
            for (int i = 0; i < 3; ++i) {
                beep(300);
                delay(350);
            }
            break;
        case BuzzerPattern::Off:
            break;
    }
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
void EdifikaNode::updateDisplay(const PresentationFrame& frame) {
    unsigned long now = millis();
    if (temporaryMessageActive && now - temporaryMessageTimestamp >= 3000) {
        temporaryMessageActive = false;
    }

    if (temporaryMessageActive) {
        statusDisplay.setMessage(temporaryMessageText);
        statusDisplay.handle(OledDisplay::SHOW_MESSAGE_COMMAND);
        return;
    }

    if (frame.headline != nullptr) {
        statusDisplay.setHeadline(frame.headline);
        statusDisplay.handle(OledDisplay::SHOW_HEADLINE_COMMAND);
        return;
    }

    if (now - lastRenderTimestamp >= DISPLAY_REFRESH_IN_MILLISECONDS) {
        lastRenderTimestamp = now;
        statusDisplay.handle(OledDisplay::SHOW_DASHBOARD_COMMAND);
    }
}
