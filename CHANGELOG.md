# Changelog

All notable changes to the Edifika ESP32 firmware are documented in this file.

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
