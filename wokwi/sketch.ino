// Edifika - nodo ESP32 en Wokwi (sin backend): DHT22 + LCD 20x4 + RFID RC522
// + rele de cerradura + LED RGB + malla 3x3 + buzzer + HC-SR04 con luces de presencia.
// Sensores y actuadores con ModestIoT; la logica vive en EdifikaNode.
// El Edge Gateway se simula dentro del ESP32 (LocalGateway): escribe "ayuda" en el Monitor serie.
//
// Tarjetas RFID (clic en el lector -> elegir "Card" -> TAP):
//   Tarjeta de Wokwi        UID                        Resultado
//   Green Card              11223344                   Concedido: "Hola Ana (Dpto 101)"
//   Blue Card               01020304                   Concedido: "Hola Luis (Dpto 202)"
//   Key Fob                 C0FFEE99                   Concedido: "Conserje"
//   Red Card                AABBCCDD                   Denegado: "Deuda pendiente"
//   Yellow Card / NFC Tag   55667788 / 04112233445566  Denegado: "Tarjeta no registrada"
//                                                      (se pueden dar de alta con "registro")
// Las asignaciones viven en LocalGateway::begin().
//
// Malla de LEDs 3x3 (una fila por GPIO; en la placa real cada fila la conmuta un NPN):
//   Fila  Color   GPIO  Se enciende cuando...
//   1     Verde   16    la cerradura esta abierta (rele energizado)
//   2     Naranja 17    la temperatura es >= 28 C
//   3     Azul    2     la humedad es >= 70 %
// Para probar las filas 2 y 3: clic en el DHT22 y subir temperatura o humedad.

#include <ModestIoT.h>
#include "EdifikaNode.h"

EdifikaNode* node = nullptr;

void setup() {
  Serial.begin(115200);
  // Se construye aqui (no como global) para que los constructores que tocan hardware
  // corran con el core ya iniciado. El rele es el primer miembro: cerradura bloqueada al arrancar.
  static EdifikaNode edifikaNode;
  node = &edifikaNode;
  node->begin();
}

void loop() {
  // El muestreo y las reacciones corren en las tareas FreeRTOS de Device;
  // aqui solo se atiende la consola del gateway simulado.
  node->maintainNetwork();
  delay(10);
}
