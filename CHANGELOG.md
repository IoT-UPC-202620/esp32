# Changelog

All notable changes to the Edifika ESP32 firmware are documented in this file.

## [Unreleased]

### Added
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
