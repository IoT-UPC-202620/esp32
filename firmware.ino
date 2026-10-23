// Edifika - nodo ESP32: DHT11 + OLED SSD1306 + RFID RC522 + rele de cerradura + LED RGB + malla 3x3 + buzzer
// Todos los sensores y actuadores se modelan con ModestIoT (src/ModestIoT); la logica vive en EdifikaNode.
// Placa: ESP32 Dev Module (core 3.x). Monitor serie a 115200.
//
// Librerias: ArduinoJson, DHT sensor library (+ Adafruit Unified Sensor), Adafruit SSD1306 (+ Adafruit GFX),
// MFRC522 y las que pide ModestIoT al compilarse completo: LiquidCrystal I2C, ESP32Servo, HX711, PubSubClient.

#include "src/devices/EdifikaNode.h"

void setup() {
  // Se construye aqui (no como global) para que los constructores que tocan hardware
  // corran con el core ya iniciado. El rele es el primer miembro: cerradura bloqueada al arrancar.
  static EdifikaNode node;
  Serial.begin(115200);
  node.begin();
}

void loop() {
  // El muestreo y las reacciones corren en las tareas FreeRTOS de Device; el loop no hace falta.
  vTaskDelete(nullptr);
}
