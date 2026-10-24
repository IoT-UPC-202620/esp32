# firmware — nodo ESP32 de Edifika

Firmware del nodo IoT de Edifika construido sobre el nano-framework **ModestIoT**. Reemplaza al sketch
`edifika_test.ino` (repo `edifika-report`, en `wokwi/firmware/edifika_test/`) con el mismo comportamiento, pero
cada sensor y actuador se modela como `Sensor` / `Actuator` de ModestIoT y la orquestación vive en un `Device`.

## Estructura

```
firmware/
├── firmware.ino                 # setup(): construye EdifikaNode y arranca el motor asíncrono
└── src/
    ├── ModestIoT/               # Framework copiado sin modificaciones
    ├── config/NodeConfig.h      # Pines y parámetros (umbrales, tiempos, polaridades, red)
    ├── config/Secrets.example.h # Plantilla de Wi-Fi y broker; copiar como Secrets.h (ignorado por git)
    ├── connectivity/WiFiStationDriver.*   # Wi-Fi          -> WiFiConnectivityDriver
    ├── gateway/CommandParser.*            # Parseo puro de comandos v1 (ArduinoJson 7)
    ├── gateway/EdgeMqttGatewayClient.*    # MQTT con Edge  -> GatewayClient
    ├── telemetry/NodeMessages.*           # Mensajes JSON  -> TelemetryPackage
    ├── sensors/RfidReader.*     # RC522            -> Sensor
    ├── actuators/Relay.*        # Relé cerradura   -> Actuator
    ├── actuators/RgbLed.*       # LED RGB (PWM)    -> Actuator
    ├── actuators/OledDisplay.*  # OLED SSD1306     -> Actuator
    ├── devices/DoorPresentation.* # Máquina de estados pura: decisiones, roles, LED
    └── devices/EdifikaNode.*    # Device: eventos de sensores -> comandos a actuadores
```

## Mapeo de componentes

| Componente | Clase | Origen |
|---|---|---|
| DHT11 (GPIO4) | `DhtSensor` | ModestIoT |
| Buzzer (GPIO25) | `Buzzer` | ModestIoT |
| Filas de la malla 3x3 (GPIO16, 17, 2) | `Led` ×3 | ModestIoT |
| Lector RFID RC522 (SPI, SS 5, RST 27) | `RfidReader : Sensor` | nuevo, sobre la clase abstracta |
| Relé de la cerradura (GPIO26, activo en bajo) | `Relay : Actuator` | nuevo, sobre la clase abstracta |
| LED RGB (GPIO13, 14, 15, ánodo común) | `RgbLed : Actuator` | nuevo, sobre la clase abstracta |
| OLED SSD1306 (I2C 0x3C) | `OledDisplay : Actuator` | nuevo, mismo patrón que `CharacterLcdDisplay` |
| Presentación y roles de puerta | `DoorPresentation` | nuevo, lógica de estado pura |
| Nodo completo | `EdifikaNode : Device` | nuevo |

ModestIoT no trae RFID, relé con polaridad invertida, RGB por PWM ni pantallas gráficas, así que esas
cuatro clases extienden `Sensor` / `Actuator` siguiendo las convenciones del framework: identificadores
`*_IDENTIFIER` con su `Command`/`Event` precreado, parámetros guardados con un setter antes del comando
(como `Buzzer::setTone`), supresión de propagación con identificador `-1` y la dirección I2C o el pin
principal guardados en `pin`.

## Cómo funciona

`Device` dispara un timer cada 10 ms y atiende en round-robin tres slots del planificador:

1. `RfidReader` ← `MEASURE_DATA_REQUESTED_EVENT` → emite `CARD_DETECTED_EVENT` (60)
2. `EdifikaNode` ← `DEVICE_TICK_EVENT` (90): el propio nodo se registra como `EventHandler` para correr
   lo que depende del tiempo (cerrar la cerradura, máquina de estados de puerta, filas de la malla, refresco de la OLED)
3. `DhtSensor` ← `MEASURE_DATA_REQUESTED_EVENT` → emite `DATA_READ_EVENT` (20) cuando cambia la lectura

Al leer una tarjeta, el nodo emite un beep corto de lectura (150 ms), muestra `Validando...` en la OLED y
publica el intento de acceso en `edifika/v1/nodes/{deviceId}/access`. El `loop()` se elimina: todo corre en
las tareas FreeRTOS de `Device`.

## Conexión a internet (Wi-Fi + MQTT)

El nodo se conecta por Wi-Fi al broker MQTT (Mosquitto) del Edge Gateway y cumple el contrato v1 de Edifika:

| Sentido | Tópico | Cuándo | Payload / Contenido |
|---|---|---|---|
| Nodo → Edge | `edifika/v1/nodes/{deviceId}/heartbeat` | Al conectar y luego **cada 10 s** | `{"v":"1","ts":"…","deviceId":"esp32-acc-01","fw":"v0.3.0"}` |
| Nodo → Edge | `edifika/v1/nodes/{deviceId}/access` | Cada tarjeta leída | `{"v":"1","ts":"…","credentialType":"RFID","credential":"04A1B2C3"}` |
| Nodo → Edge | `edifika/v1/nodes/{deviceId}/readings` | **Cada 10 s** con el último valor válido | `{"v":"1","ts":"…","sensor":"temperature","value":24.5,"unit":"°C"}` |
| Nodo → Edge | `edifika/v1/nodes/{deviceId}/ack` | Respuesta obligatoria a cada comando con `commandId` | `{"v":"1","ts":"…","commandId":"…","status":"OK","detail":""}` (o `FAILED`) |
| Edge → Nodo | `edifika/v1/nodes/{deviceId}/commands` | Órdenes del gateway | `access_result`, `mode`, `rgb`, `unlock`, `display`, `buzzer`, `alert`, `maintenance`, `time_sync` |

Todos los mensajes llevan `"v":"1"` y `ts` en ISO-8601 UTC sincronizado por NTP; mientras el reloj no está
sincronizado se envía `ts: null` en vez de una hora inventada.

Qué se usa de ModestIoT y qué se extendió:

- `WiFiConnectivityDriver` deja `transmit()` sin implementar (la clase es abstracta), así que
  `WiFiStationDriver` lo hereda tal cual y solo completa `transmit()` (envío TCP puntual a `"host:port"`).
- `MqttGatewayClient` publica en un único tópico fijo y no se suscribe, así que `EdgeMqttGatewayClient`
  extiende `GatewayClient` para publicar cada mensaje en su canal y recibir `/commands`.
- Los mensajes son `TelemetryPackage` y viajan por la cola de telemetría de `Device`: se encolan en la tarea
  de hardware y se publican en `processQueuedTelemetryData()` (tarea NetworkWorker).
- `loop()` mantiene la sesión (`maintainNetwork()`, reintento cada 5 s). Los comandos recibidos pasan por una
  cola FreeRTOS (capacidad 8) y se ejecutan en el tick del nodo, garantizando que los actuadores solo se tocan
  desde una tarea.

Configuración:

1. `cp src/config/Secrets.example.h src/config/Secrets.h` y completar SSID, contraseña e IP del broker.
2. En `NodeConfig.h`: `DEVICE_ID = "esp32-acc-01"`, `UNLOCK_ON_ANY_CARD = false`.

## Comportamiento de la puerta (Contratos §1.3)

- **Lectura de tarjeta:**
  1. El nodo publica `access`, emite un beep corto (150 ms) y muestra `Validando...` en la OLED.
  2. No abre por su cuenta (`UNLOCK_ON_ANY_CARD = false`).
  3. Espera `access_result` hasta 5 s (`ACCESS_RESULT_TIMEOUT_IN_MILLISECONDS`).
  4. Si no llega respuesta, deniega localmente y muestra `Gateway no disponible` (US47) con sonido de denegado y LED rojo.
  5. Las lecturas repetidas de cualquier tarjeta dentro de esa espera de 5 s se ignoran.
  6. Si llega un `access_result` tardío (después de vencer el timeout), se descarta y la cerradura no se abre.
- **Decisión del gateway (`access_result`):**
  - Si `result == "GRANTED"`: abre el relé durante `lockMs` (o valor por defecto), reproduce el patrón de buzzer `granted`, muestra `Acceso concedido` (o el mensaje/nombre del titular) en la OLED y enciende el LED en verde `{0,255,0}` durante 2 s (si está en modo `FREE_ACCESS`).
  - Si `result == "DENIED"`: reproduce el patrón de buzzer `denied`, muestra `Acceso denegado` en la OLED y enciende el LED en rojo `{255,0,0}` durante 2 s (si está en modo `FREE_ACCESS`).
- **LED RGB de estado y roles:**
  - **Sin conexión:** rojo fijo `{255,0,0}` desde el arranque hasta recibir confirmación de sesión MQTT con el primer heartbeat.
  - **Modo `FREE_ACCESS`:** blanco `{255,255,255}` en reposo; conmuta temporalmente a verde o rojo durante 2 s tras cada decisión de acceso.
  - **Modo `RESERVED`:** muestra de forma fija el color asignado a la reserva (`#00FF88`, etc.). La pantalla OLED muestra `RESERVADO`. Tras una decisión de acceso concedido o denegado, el LED mantiene el color de la reserva para preservar la identidad visual del área reservada. Un comando `rgb` solo se acepta en modo `RESERVED` (en `FREE_ACCESS` responde `FAILED "not reserved"`).
  - **Mantenimiento:** color ámbar `{255,128,0}` (`#FF8000`). La OLED muestra el mensaje de mantenimiento y el lector ignora cualquier lectura de tarjeta.
- **Malla de LEDs 3x3:**
  - Fila 1: se enciende mientras la cerradura eléctrica permanece energizada.
  - Fila 2: se enciende cuando la temperatura ambiente es ≥ 28.0 °C.
  - Fila 3: se enciende cuando la humedad relativa es ≥ 70 %.
- **Arranque:** el modo inicial es `FREE_ACCESS`. Al reconectar con el broker MQTT, el Edge Gateway envía automáticamente el comando `mode` correspondiente si hay una reserva activa.

## Registro del nodo en la nube

Para dar de alta el gateway y el nodo en la plataforma cloud (`IoT Access Management` en el puerto 8089):

1. **Registrar el Edge Gateway:**
   ```http
   POST /api/v1/iot/gateways
   Content-Type: application/json
   Authorization: Bearer <JWT_ADMIN>

   {
     "gatewayCode": "edge-gateway-01",
     "buildingId": 12,
     "name": "Gateway Edificio Principal"
   }
   ```

2. **Registrar el dispositivo (puerta con cerradura y sensores):**
   ```http
   POST /api/v1/iot/devices
   Content-Type: application/json
   Authorization: Bearer <JWT_ADMIN>

   {
     "deviceCode": "esp32-acc-01",
     "gatewayId": 1,
     "buildingId": 12,
     "name": "Puerta Principal / Acceso",
     "location": "Piso 1",
     "deviceType": "DOOR",
     "capabilities": ["RFID", "LOCK", "BUZZER", "OLED", "RGB_LED", "TEMPERATURE", "HUMIDITY"]
   }
   ```

## Broker MQTT: Desarrollo e Integración

### Broker de desarrollo local (en este repositorio)

`broker/` levanta la imagen oficial `eclipse-mosquitto:2` en el puerto 1883, sin usuarios ni TLS (solo para desarrollo local).

```bash
cd broker && docker compose up -d     # levantar broker
docker logs -f edifika-mqtt-logger    # telemetria del nodo
./broker/watch.sh                     # visor interactivo de eventos
./broker/send-command.sh unlock       # enviar comando unlock
./broker/send-command.sh mode         # enviar comando mode
```

Consulte `broker/README.md` para ver la lista completa de scripts de prueba.

### Broker de integración (Edifika-IoT-Platform)

En integración punta a punta, el broker Mosquitto se ejecuta dentro de `Edifika-IoT-Platform/docker-compose.yml`, conectado al Edge Gateway en Python y a RabbitMQ.

## Tests en el host

Para correr la suite de tests unitarios puros (parseo de comandos, máquina de estados de puerta) sin depender del hardware ni de Arduino:

```bash
bash test/run_tests.sh
```

Requiere `g++` (C++17) y `bash` en el PATH.

## Compilar

Placa **ESP32 Dev Module**, core **esp32 3.x** (ModestIoT usa la API de timers de 3.x).

Librerías: `ArduinoJson` (v7), `PubSubClient`, `DHT sensor library`, `Adafruit Unified Sensor`, `Adafruit SSD1306`, `Adafruit GFX Library`,
`MFRC522`. Como el IDE compila todo `src/`, ModestIoT completo también pide `LiquidCrystal I2C`, `ESP32Servo`
y `HX711`, aunque este nodo no los use.

```bash
arduino-cli lib install ArduinoJson "DHT sensor library" "Adafruit Unified Sensor" "Adafruit SSD1306" \
  "Adafruit GFX Library" MFRC522 "LiquidCrystal I2C" ESP32Servo HX711 PubSubClient
arduino-cli compile -b esp32:esp32:esp32 .
arduino-cli upload  -b esp32:esp32:esp32 -p <puerto> .
```

En Arduino IDE basta con abrir `firmware.ino` (después de crear `src/config/Secrets.h`). Con Wi-Fi el binario
ocupa ~76 % de la partición por defecto; si crece, elegir el esquema de partición *Huge APP*.

## Diferencias con `edifika_test.ino`

- Se eliminó el degradado RGB continuo en favor de la máquina de estados de roles y decisiones del contrato v1 (`DoorPresentation`).
- Las lecturas de telemetría de temperatura y humedad se publican periódicamente cada 10 s con el último valor válido en vez de dispararse en cada muestra del sensor DHT.
- La unidad de temperatura es `"°C"` conforme al contrato v1.
- El intervalo de heartbeat se redujo a 10 s para una rápida detección de desconexión por parte del Edge Gateway (timeout a 30 s).
- La apertura ya no es autónoma (`UNLOCK_ON_ANY_CARD = false`): la autorización la realiza el Edge Gateway centralmente, con fallback local a los 5 s (`Gateway no disponible`) si el gateway no responde.
- Todos los comandos recibidos responden por el canal `ack` con estado `"OK"` o `"FAILED"`.
- Soporte para los 9 comandos remotos del contrato v1.

## Checklist de aceptación manual (con hardware)

La ejecuta Jonatan con la placa física y el stack del Plan 5 levantado:
1. **Encendido:** LED rojo al arrancar y luego blanco cuando el gateway acepta el primer heartbeat.
2. **Tarjeta registrada y al día:** la OLED muestra `Validando...`, luego `Acceso concedido`, el LED se ilumina verde 2 s, el relé abre 5 s (`lockMs`) y la fila 1 de la malla se enciende.
3. **Tarjeta de residente moroso:** la OLED muestra `Acceso denegado`, el LED se ilumina rojo 2 s, sin apertura de relé.
4. **Reserva desde la API:** la OLED muestra `RESERVADO` y el LED adopta el color fijado para la reserva.
5. **`PUT /led` como titular:** el color del LED cambia en menos de 3 s.
6. **Gateway apagado y lectura de tarjeta:** a los 5 s de espera, la OLED muestra `Gateway no disponible`.
7. **Fin de la reserva:** el LED vuelve a blanco en reposo.

## Diagrama del circuito

![Circuito del nodo](docs/circuito.png)

`docs/circuito.mmd` (Mermaid) se genera con `docs/gen_circuit.py`: un nodo por pin y una línea por cable,
coloreada por tipo de señal. Las etiquetas `+3V3`, `+5V` y `GND` repetidas son la misma red. Para regenerarlo:

```bash
python3 docs/gen_circuit.py docs/circuito.mmd
npx -y @mermaid-js/mermaid-cli@11.4.2 -i docs/circuito.mmd -o docs/circuito.png -w 4000 -s 2 -b white
```
