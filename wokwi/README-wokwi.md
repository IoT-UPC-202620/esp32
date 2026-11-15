# Nodo Edifika en Wokwi (sin backend)

Versión del firmware para el simulador [Wokwi](https://wokwi.com). Usa ModestIoT desde la librería
`ModestIoT@wokwi:8c6d7d14…` (ver `libraries.txt`), así que esta carpeta no copia `src/ModestIoT`.

Para cargarla en Wokwi: proyecto ESP32 nuevo → menú de archivos → *Upload file(s)…* con todos los
archivos de esta carpeta (reemplazando `sketch.ino` y `diagram.json`).

## Diferencias con la placa real

| Placa real | Wokwi | Por qué |
|---|---|---|
| OLED SSD1306 (`OledDisplay`) | LCD 20x4 I2C 0x27 (`LcdStatusDisplay` sobre `CharacterLcdDisplay`) | Adafruit SSD1306/GFX no están en la lista de librerías |
| DHT11 | DHT22 | Wokwi no tiene DHT11 |
| Relé activo en bajo | Relé activo en alto | Así funciona el módulo de relé de Wokwi |
| Fundido LEDC por hardware (`ledcFade`) | Fundido por software en `DimmableLed::update` | `ledcFade` bloquea la simulación |
| Wi-Fi + MQTT con el Edge Gateway | `LocalGateway`: gateway simulado en el propio ESP32 | Wokwi no llega al broker local |

`RfidReader` (RC522 con la librería MFRC522, agregada a `libraries.txt`), `CommandParser`, `DoorPresentation`, `PresenceLight`, `NodeIdentity`, `NodeMessages`, `Relay` y `RgbLed`
son los mismos de `src/`, solo cambian las rutas de `#include`.

## Gateway simulado

Todo lo que el nodo publica se imprime en el Monitor serie con su tópico v1, y el gateway reacciona como el
real: adopta el nodo con `mode` al recibir `announce`, responde `access_result` a cada tarjeta, da de alta las
tarjetas de `enrollment` y reenvía `mode` si el heartbeat no coincide con la configuración.

Para pasar una tarjeta: clic en el lector RFID → elegir la tarjeta en *Card* → **TAP**.

| Tarjeta de Wokwi | UID | Respuesta |
|---|---|---|
| Green Card | `11223344` | GRANTED "Hola Ana (Dpto 101)", abre 5 s |
| Blue Card | `01020304` | GRANTED "Hola Luis (Dpto 202)" |
| Key Fob | `C0FFEE99` | GRANTED "Conserje" |
| Red Card | `AABBCCDD` | DENIED "Deuda pendiente" |
| Yellow Card / NFC Tag | `55667788` / `04112233445566` | DENIED "Tarjeta no registrada" (hasta registrarlas con `registro`) |

Comandos por el Monitor serie: `ayuda`, `reservar [#RRGGBB]`, `libre`, `color #RRGGBB`,
`mant on [mensaje]` / `mant off`, `abrir [ms]`, `msg <texto>`, `alerta [texto]`, `buzzer granted|denied|alert`,
`registro [off]`, `identificar`, `offline` / `online` (simula la caída del gateway: la tarjeta vence a los 5 s
con "Gateway no disponible"), `tarjetas`, o un comando v1 en JSON tal cual.

