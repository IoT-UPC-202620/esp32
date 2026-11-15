#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

/**
 * @file NodeConfig.h
 * @brief Mapa de pines y parametros del nodo Edifika en Wokwi.
 *
 * Diferencias con la placa real (ver README-wokwi.md):
 *  - DHT22 en vez de DHT11 (Wokwi no simula el DHT11).
 *  - LCD 20x4 I2C (CharacterLcdDisplay de ModestIoT) en vez de la OLED SSD1306.
 *  - Sin Wi-Fi ni MQTT: un gateway local simulado (LocalGateway) responde por el Monitor serie.
 */

#include <Arduino.h>
#include <DHT.h>

namespace NodeConfig {

// ---------------- DHT22 ----------------
constexpr int DHT_PIN = 4;
constexpr uint8_t DHT_MODEL = DHT22;
constexpr float HIGH_TEMPERATURE_IN_CELSIUS = 28.0f;   // fila 2 de la malla
constexpr float HIGH_HUMIDITY_IN_PERCENTAGE = 70.0f;   // fila 3 de la malla

// ---------------- LCD 20x4 I2C (SDA 21, SCL 22) ----------------
constexpr uint8_t LCD_I2C_ADDRESS = 0x27;
constexpr uint8_t LCD_COLUMNS = 20;
constexpr uint8_t LCD_ROWS = 4;

// ---------------- RFID RC522 (SPI, pines por defecto de VSPI) ----------------
constexpr int RFID_SCK_PIN = 18;
constexpr int RFID_MISO_PIN = 19;
constexpr int RFID_MOSI_PIN = 23;
constexpr int RFID_SS_PIN = 5;      // pin rotulado SDA en el modulo
constexpr int RFID_RESET_PIN = 27;

// ---------------- Buzzer ----------------
constexpr int BUZZER_PIN = 25;
constexpr int BUZZER_FREQUENCY_IN_HERTZ = 2000;
constexpr unsigned long STARTUP_BEEP_DURATION_IN_MILLISECONDS = 300;

// ---------------- Rele de la cerradura ----------------
// El modulo de rele de Wokwi se activa con HIGH (el de la placa real, con LOW).
constexpr int LOCK_RELAY_PIN = 26;
constexpr bool LOCK_RELAY_ACTIVE_LOW = false;
constexpr unsigned long LOCK_OPEN_DURATION_IN_MILLISECONDS = 3000;

// ---------------- LED RGB (anodo comun) ----------------
constexpr int RGB_RED_PIN = 13;
constexpr int RGB_GREEN_PIN = 14;
constexpr int RGB_BLUE_PIN = 15;
constexpr bool RGB_COMMON_ANODE = true;
constexpr uint32_t RGB_PWM_FREQUENCY_IN_HERTZ = 5000;

// ---------------- Malla de LEDs 3x3 (una fila por GPIO) ----------------
constexpr uint8_t MATRIX_ROW_COUNT = 3;
constexpr int MATRIX_ROW_PINS[MATRIX_ROW_COUNT] = {16, 17, 2};

// ---------------- Luz de presencia: HC-SR04 + 3 LEDs blancos ----------------
constexpr int ULTRASONIC_TRIGGER_PIN = 32;
constexpr int ULTRASONIC_ECHO_PIN = 34;
constexpr int PRESENCE_LIGHT_PIN = 33;
constexpr float PRESENCE_DISTANCE_IN_CENTIMETERS = 80.0f;
constexpr uint8_t PRESENCE_REQUIRED_HITS = 3;
constexpr unsigned long PRESENCE_HOLD_IN_MILLISECONDS = 10000;
constexpr uint8_t PRESENCE_LIGHT_BRIGHTNESS = 255;
constexpr uint32_t PRESENCE_FADE_IN_IN_MILLISECONDS = 400;
constexpr uint32_t PRESENCE_FADE_OUT_IN_MILLISECONDS = 1500;
constexpr uint32_t PRESENCE_PWM_FREQUENCY_IN_HERTZ = 5000;
constexpr bool PRESENCE_DEBUG_LOG = false;   // true: imprime la distancia cada segundo
constexpr unsigned long PRESENCE_DEBUG_LOG_INTERVAL_IN_MILLISECONDS = 1000;

// ---------------- Pantalla y planificador ----------------
constexpr unsigned long DISPLAY_REFRESH_IN_MILLISECONDS = 500;
constexpr unsigned long SPLASH_DURATION_IN_MILLISECONDS = 1500;
constexpr unsigned long TEMPORARY_MESSAGE_IN_MILLISECONDS = 3000;
// Periodo del timer de Device: 4 slots (RFID, reloj, DHT, ultrasonico) -> cada uno cada 40 ms
constexpr unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 10;

// ---------------- Contrato v1 (gateway local simulado) ----------------
constexpr const char* DEVICE_ID_OVERRIDE = "esp32-acc-01";
constexpr const char* DEVICE_MODEL = "door-node";
constexpr const char* FIRMWARE_VERSION = "v0.6.0-wokwi";
constexpr unsigned long ANNOUNCE_INTERVAL_IN_MILLISECONDS = 60000;
constexpr unsigned long HEARTBEAT_INTERVAL_IN_MILLISECONDS = 10000;
constexpr unsigned long MIN_HEARTBEAT_GAP_IN_MILLISECONDS = 1000;
constexpr unsigned long READINGS_INTERVAL_IN_MILLISECONDS = 10000;
constexpr unsigned long ACCESS_RESULT_TIMEOUT_IN_MILLISECONDS = 5000;
constexpr unsigned long DECISION_FEEDBACK_IN_MILLISECONDS = 2000;
// Demora con la que el gateway simulado responde, para que se vea "Validando..."
constexpr unsigned long LOCAL_GATEWAY_LATENCY_IN_MILLISECONDS = 700;
// Apertura que ordena el gateway simulado al conceder acceso (lockMs)
constexpr unsigned long LOCAL_GATEWAY_LOCK_IN_MILLISECONDS = 5000;

// true: cualquier tarjeta abre en el acto. false: abre solo si el gateway responde GRANTED.
constexpr bool UNLOCK_ON_ANY_CARD = false;

} // namespace NodeConfig

#endif // NODE_CONFIG_H
