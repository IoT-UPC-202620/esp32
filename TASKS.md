# Tasks — Firmware esp32 (H6)

## Task 1 — Gitflow y arnés de tests en el host   [x] hecha
- Rama: feature/H6-host-test-harness
- Paso actual: Step 3 — Commit y merge a develop
- Archivos tocados: .gitignore, README.md, test/vendor/ArduinoJson.h, test/minitest.h, test/test_main.cpp, test/run_tests.sh, TASKS.md
- Tests: `bash test/run_tests.sh` → 1 passed, 0 failed
- Último commit: 4b59220 test: add host-side test harness with vendored ArduinoJson (H6)
- Métricas: 1 host test en verde
- Bloqueos / notas: —

## Task 2 — CommandParser: todos los comandos del contrato   [x] hecha
- Rama: feature/H6-command-parser
- Paso actual: Step 5 — Commit y merge a develop
- Archivos tocados: src/gateway/CommandParser.h, src/gateway/CommandParser.cpp, src/gateway/EdgeMqttGatewayClient.h, src/gateway/EdgeMqttGatewayClient.cpp, test/test_command_parser.cpp, TASKS.md
- Tests: `bash test/run_tests.sh` → 13 passed, 0 failed
- Último commit: c743499 feat(gateway): parse every v1 command into a typed NodeCommand (H6)
- Métricas: 13 host tests en verde (12 de CommandParser)
- Bloqueos / notas: —

## Task 3 — DoorPresentation: decisión, modo y LED como máquina de estados pura   [x] hecha
- Rama: feature/H6-door-presentation
- Paso actual: Step 5 — Commit y merge a develop
- Archivos tocados: src/devices/DoorPresentation.h, src/devices/DoorPresentation.cpp, test/test_door_presentation.cpp, TASKS.md
- Tests: `bash test/run_tests.sh` → 24 passed, 0 failed
- Último commit: 424d461 feat(devices): pure door presentation state machine for decision, mode and LED (H6)
- Métricas: 24 host tests en verde (11 de DoorPresentation)
- Bloqueos / notas: —

## Task 4 — EdifikaNode conectado al contrato   [x] hecha
- Rama: feature/H6-node-contract
- Paso actual: Step 3 — Commit y merge a develop
- Archivos tocados: src/config/NodeConfig.h, src/actuators/OledDisplay.h, src/actuators/OledDisplay.cpp, src/telemetry/NodeMessages.cpp, src/devices/EdifikaNode.h, src/devices/EdifikaNode.cpp, TASKS.md
- Tests: `bash test/run_tests.sh` → 24 passed, 0 failed
- Último commit: 449fe23 feat(node): gateway-driven access, door roles, RGB status and periodic readings (H6)
- Métricas: 24 host tests en verde; 9 comandos del contrato despachados con ack OK/FAILED
- Bloqueos / notas: —

## Task 5 — Broker del repo, prueba con Mosquitto y documentación   [x] hecha
- Rama: feature/H6-broker-and-docs
- Paso actual: Step 3 — Commit y merge a develop
- Archivos tocados: broker/send-command.sh, broker/README.md, README.md, TASKS.md
- Tests: `grep -oE '"type":"[a-z_]+"' broker/send-command.sh | sort -u` → 9 tipos de comandos verificados; `bash test/run_tests.sh` → 24 passed, 0 failed
- Último commit: 3696f96 docs: document the v1 contract behaviour, node registration and command examples (H6)
- Métricas: 9 comandos documentados y ejemplificados con mosquitto_pub
- Bloqueos / notas: —

## Task 6 — Compilación de la placa y release   [x] hecha
- Rama: release/0.3.0
- Paso actual: Step 3 — Merge a main y develop, y tag v0.3.0
- Archivos tocados: src/devices/DoorPresentation.h, src/config/NodeConfig.h, CHANGELOG.md, TASKS.md
- Tests: `arduino-cli compile -b esp32:esp32:esp32 .` → exit 0 (Flash: 1,013,484 B / 77%, RAM: 51,308 B / 15%); `bash test/run_tests.sh` → 24 passed, 0 failed
- Último commit: 3696f96 docs: document the v1 contract behaviour, node registration and command examples (H6)
- Métricas: Compilación de placa verificada con éxito en ESP32 Dev Module (core 3.3.12); 24 host tests en verde
- Bloqueos / notas: Permiso de instalación concedido por Jonatan ejecutado con éxito.

## Task 7 — Checklist de aceptación manual con hardware   [!] bloqueada
- Rama: release/0.3.0
- Paso actual: Documentada en README.md ("Checklist de aceptación manual"), pendiente de ejecución por Jonatan
- Archivos tocados: README.md
- Tests: Pendiente de prueba con hardware físico y stack de integración
- Último commit: —
- Métricas: 7 escenarios de aceptación documentados
- Bloqueos / notas: Requiere placa física ESP32 y credenciales reales; reservada para Jonatan.

## Task 8 — RGB LED only lit in RESERVED (change requested by Jonatan, 2026-10-24)   [x] hecha
- Rama: feature/H6-led-off-outside-reserved
- Paso actual: merged into develop; released as v0.4.0
- Archivos tocados: src/devices/DoorPresentation.h/.cpp, src/devices/EdifikaNode.cpp, test/test_door_presentation.cpp, README.md, CHANGELOG.md
- Tests: `bash test/run_tests.sh` → 26 passed, 0 failed; `arduino-cli compile -b esp32:esp32:esp32` → exit 0 (77 % flash)
- Hardware (COM5): uploaded; LED off in FREE_ACCESS; `PUT /led` returns 409 outside RESERVED; magenta `#FF0080` applied during a reservation (command OK)
- Notas: outside RESERVED the LED is off with no exceptions (no green/red decision flash, no amber maintenance, no red without broker); in RESERVED it keeps the reservation colour even without broker, and turns off in maintenance.
