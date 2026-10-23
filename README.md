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
    ├── config/NodeConfig.h      # Pines y parámetros (umbrales, tiempos, polaridades)
    ├── sensors/RfidReader.*     # RC522            -> Sensor
    ├── actuators/Relay.*        # Relé cerradura   -> Actuator
    ├── actuators/RgbLed.*       # LED RGB (PWM)    -> Actuator
    ├── actuators/OledDisplay.*  # OLED SSD1306     -> Actuator
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
   lo que depende del tiempo (cerrar la cerradura a los 3 s, degradado RGB, filas de la malla, refresco de la OLED)
3. `DhtSensor` ← `MEASURE_DATA_REQUESTED_EVENT` → emite `DATA_READ_EVENT` (20) cuando cambia la lectura

Al leer una tarjeta, el nodo pita, ejecuta `UNLOCK_DOOR_COMMAND` (400), que también puede llegar desde
afuera más adelante, y muestra la tarjeta 3 s en la OLED. El `loop()` se elimina: todo corre en las tareas
FreeRTOS de `Device`.

## Compilar

Placa **ESP32 Dev Module**, core **esp32 3.x** (ModestIoT usa la API de timers de 3.x).

Librerías: `ArduinoJson`, `DHT sensor library`, `Adafruit Unified Sensor`, `Adafruit SSD1306`, `Adafruit GFX Library`,
`MFRC522`. Como el IDE compila todo `src/`, ModestIoT completo también pide `LiquidCrystal I2C`, `ESP32Servo`,
`HX711` y `PubSubClient`, aunque este nodo no los use.

```bash
arduino-cli lib install ArduinoJson "DHT sensor library" "Adafruit Unified Sensor" "Adafruit SSD1306" \
  "Adafruit GFX Library" MFRC522 "LiquidCrystal I2C" ESP32Servo HX711 PubSubClient
arduino-cli compile -b esp32:esp32:esp32 .
arduino-cli upload  -b esp32:esp32:esp32 -p <puerto> .
```

En Arduino IDE basta con abrir `firmware.ino`.

## Diferencias con `edifika_test.ino`

- `DhtSensor` descarta las lecturas fallidas o repetidas: la OLED conserva el último valor válido en vez de
  mostrar `--.-` tras un error, y `T=… H=…` solo se imprime cuando la lectura cambia.
- El degradado RGB se calcula a partir de `millis()` (mismo recorrido de ~5 s por tramo) porque el tick
  del nodo llega cada 30 ms, no cada 10 ms.

## Diagrama del circuito

![Circuito del nodo](docs/circuito.png)

`docs/circuito.mmd` (Mermaid) se genera con `docs/gen_circuit.py`: un nodo por pin y una línea por cable,
coloreada por tipo de señal. Las etiquetas `+3V3`, `+5V` y `GND` repetidas son la misma red. Para regenerarlo:

```bash
python3 docs/gen_circuit.py docs/circuito.mmd
npx -y @mermaid-js/mermaid-cli@11.4.2 -i docs/circuito.mmd -o docs/circuito.png -w 4000 -s 2 -b white
```
