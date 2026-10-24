#ifndef SECRETS_H
#define SECRETS_H

/**
 * @file Secrets.example.h
 * @brief Plantilla de credenciales. Copiala como Secrets.h (ignorado por git) y completa tus datos.
 */

#include <Arduino.h>

namespace Secrets {

// En Wokwi la red simulada es "Wokwi-GUEST" sin contrasena.
constexpr const char* WIFI_SSID = "Wokwi-GUEST";
constexpr const char* WIFI_PASSWORD = "";

// Broker MQTT del Edge Gateway (Mosquitto en el edificio).
constexpr const char* MQTT_BROKER_HOST = "192.168.1.10";
constexpr uint16_t MQTT_BROKER_PORT = 1883;

} // namespace Secrets

#endif // SECRETS_H
