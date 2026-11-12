# Changelog

All notable changes to the Edifika ESP32 firmware are documented in this file.

## [Unreleased]

### Added
- Presence lights: an HC-SR04 (TRIG GPIO32, ECHO GPIO34 through a 1k/2k divider) turns on three white LEDs in parallel, switched by a BC548 on GPIO33. They turn on after 3 consecutive readings under 80 cm, stay on 10 s after the last detection and fade in and out in hardware with `ledcFade` (`DimmableLed`). They work even before the node is adopted.
- Host tests for the presence logic (`test/test_presence_light.cpp`).
- Startup flash of the presence lights (wiring check) and `PRESENCE_DEBUG_LOG`, which prints the last distance every second.

### Changed
- The OLED dashboard shows the HC-SR04 distance (cm) and the presence lights state (ON/OFF) instead of the last card UID. The "TARJETA DETECTADA" screen still appears when a card is read.
- Device discovery (v0.6.0): the device id comes from the chip MAC (`esp32-xxxxxx`), so one binary serves every board; `DEVICE_ID_OVERRIDE` keeps a fixed id for boards registered before discovery.
- The node publishes `announce` (MAC, firmware, model, detected capabilities, IP, RSSI and a 4-digit pairing code) on every broker session and, while not adopted, every minute.
- Until the gateway sends its first `mode` the node is inert: cards are ignored, no readings are published and the OLED shows "Sin registrar" with the pairing code.
- `identify {seconds?}` (1..60, 10 by default) blinks the RGB LED white and beeps, also before adoption.
- Host tests for the identity, the unadopted state and `identify` (`test/test_discovery.cpp`).
- Card enrollment mode: the `enroll` command (`{active, seconds?}`, 60 s by default, clamped to 1..300) puts the reader in `Enrolling`: cyan LED and `MODO REGISTRO` on the OLED. The next card is published on the new `enrollment` channel (never on `access`), the lock stays closed and the node shows `Tarjeta capturada`. The mode ends on its own when the window passes; it is rejected during maintenance (`ack FAILED "maintenance"`).
- Host tests for the `enroll` command and the enrollment phase (`test/test_enrollment.cpp`).

## [v0.5.0] - 2026-10-24 (H6)

### Added
- Heartbeats report the door `mode` and the LED `color` (null outside RESERVED) so the edge gateway can re-send the latest configuration when the node is stale.
- A heartbeat is sent as soon as a new MQTT session starts (boot or reconnection), spaced at least `MIN_HEARTBEAT_GAP_IN_MILLISECONDS`.
- `CommandParser::formatHexColor` and `DoorPresentation::reservedLedColor`, with host tests (28 in total).

## [v0.4.0] - 2026-10-24 (H6)

### Changed
- The RGB LED is only lit while the door is `RESERVED`; in every other state (free access, after an access decision, maintenance, no broker session, boot) it stays off. The OLED and buzzer carry the feedback.

## [v0.3.0] - 2026-10-24 (H6)

### Added
- Pure `CommandParser` supporting all 9 MQTT v1 remote commands (`access_result`, `mode`, `rgb`, `unlock`, `display`, `buzzer`, `alert`, `maintenance`, `time_sync`).
- Pure `DoorPresentation` state machine managing door visual/acoustic feedback, operational roles (`FREE_ACCESS`, `RESERVED`), maintenance mode and decision timeouts.
- Host test suite executed via `bash test/run_tests.sh` with 24 green tests.
- Support for `SHOW_HEADLINE_COMMAND` (324) and `setHeadline` in `OledDisplay`.
- Periodic readings for temperature and humidity published every 10 seconds with unit `°C`.
- Command acknowledgment publishing (`OK` / `FAILED`) on `edifika/v1/nodes/{deviceId}/ack`.
- Test script `broker/send-command.sh` covering all 9 command types with `mosquitto_pub`.
- Acceptance checklist for physical hardware testing.
- Successful ESP32 board compilation with `arduino-cli compile` using core 3.3.12 (program storage: 1,013,484 bytes / 77%, dynamic memory: 51,308 bytes / 15%).

### Changed
- `FIRMWARE_VERSION` bumped to `v0.3.0`.
- `HEARTBEAT_INTERVAL_IN_MILLISECONDS` set to 10000 ms.
- `UNLOCK_ON_ANY_CARD` set to `false`: door access is centrally authorized by Edge Gateway.
- Replaced continuous RGB gradient with role-based LED status (white idle, green/red decision, fixed reservation color, amber maintenance, solid red when disconnected).
