#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

/**
 * @file NodeConfig.h
 * @brief Mapa de pines y parametros del nodo ESP32 de Edifika.
 *
 * Placa: ESP32 Dev Module (core 3.x). Monitor serie a 115200.
 */

#include <Arduino.h>
#include <DHT.h>

namespace NodeConfig {

// ---------------- DHT11 ----------------
constexpr int DHT_PIN = 4;
constexpr uint8_t DHT_MODEL = DHT11;
constexpr float HIGH_TEMPERATURE_IN_CELSIUS = 28.0f;   // fila 2 de la malla
constexpr float HIGH_HUMIDITY_IN_PERCENTAGE = 70.0f;   // fila 3 de la malla

// ---------------- OLED SSD1306 (I2C: SDA 21, SCL 22) ----------------
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;

// ---------------- RFID RC522 (SPI, pines por defecto de VSPI) ----------------
constexpr int RFID_SCK_PIN = 18;
constexpr int RFID_MISO_PIN = 19;
constexpr int RFID_MOSI_PIN = 23;
constexpr int RFID_SS_PIN = 5;      // pin rotulado SDA en el modulo
constexpr int RFID_RESET_PIN = 27;

// ---------------- Buzzer pasivo (tambien suena con uno activo) ----------------
constexpr int BUZZER_PIN = 25;
constexpr int BUZZER_FREQUENCY_IN_HERTZ = 2000;
constexpr unsigned long BEEP_DURATION_IN_MILLISECONDS = 500;
constexpr unsigned long STARTUP_BEEP_DURATION_IN_MILLISECONDS = 300;

// ---------------- Rele de la cerradura (modulo SRD-05VDC-SL-C, IN1) ----------------
// La mayoria de estos modulos se activan con LOW. Si el tuyo hace clic con HIGH, pon false.
constexpr int LOCK_RELAY_PIN = 26;
constexpr bool LOCK_RELAY_ACTIVE_LOW = true;
constexpr unsigned long LOCK_OPEN_DURATION_IN_MILLISECONDS = 3000;

// ---------------- LED RGB de 4 patas (220 ohm por color) ----------------
constexpr int RGB_RED_PIN = 13;
constexpr int RGB_GREEN_PIN = 14;
constexpr int RGB_BLUE_PIN = 15;
constexpr bool RGB_COMMON_ANODE = true;     // el de este montaje enciende con la pata larga a 3V3
constexpr uint32_t RGB_PWM_FREQUENCY_IN_HERTZ = 5000;
constexpr unsigned long RGB_STEP_IN_MILLISECONDS = 10;   // 510 pasos x 10 ms = ~5 s de rojo a azul

// ---------------- Malla de LEDs 3x3 ----------------
// Cada fila la conmuta un NPN: GPIO -> 1k -> base. HIGH = fila encendida.
constexpr uint8_t MATRIX_ROW_COUNT = 3;
constexpr int MATRIX_ROW_PINS[MATRIX_ROW_COUNT] = {16, 17, 2};   // GPIO2 tambien enciende el LED azul de la placa

// ---------------- Pantalla y planificador ----------------
constexpr unsigned long CARD_SCREEN_DURATION_IN_MILLISECONDS = 3000;
constexpr unsigned long DISPLAY_REFRESH_IN_MILLISECONDS = 500;
constexpr unsigned long SPLASH_DURATION_IN_MILLISECONDS = 1500;

// Periodo del timer de Device. Cada tick atiende un slot en round-robin,
// con 3 slots (RFID, reloj del nodo, DHT) cada uno se atiende cada 30 ms.
constexpr unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 10;

// ---------------- Red (Wi-Fi + MQTT con el Edge Gateway) ----------------
// Credenciales y broker en Secrets.h. Topicos: edifika/v1/nodes/{DEVICE_ID}/{canal}
constexpr const char* DEVICE_ID = "esp32-acc-01";
constexpr const char* FIRMWARE_VERSION = "v0.3.0";
constexpr unsigned long HEARTBEAT_INTERVAL_IN_MILLISECONDS = 10000;
constexpr unsigned long READINGS_INTERVAL_IN_MILLISECONDS = 10000;
constexpr unsigned long ACCESS_RESULT_TIMEOUT_IN_MILLISECONDS = 5000;
constexpr unsigned long DECISION_FEEDBACK_IN_MILLISECONDS = 2000;
constexpr const char* NTP_SERVER = "pool.ntp.org";   // hora UTC para el campo "ts"

// true: cualquier tarjeta abre en el acto (modo prueba). false: solo abre cuando el
// Edge Gateway responde con el comando "unlock" tras validar la tarjeta.
constexpr bool UNLOCK_ON_ANY_CARD = false;

} // namespace NodeConfig

#endif // NODE_CONFIG_H
