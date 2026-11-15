#ifndef LOCAL_GATEWAY_H
#define LOCAL_GATEWAY_H

/**
 * @file LocalGateway.h
 * @brief Edge Gateway simulado dentro del propio ESP32 (Wokwi, sin backend).
 *
 * Reemplaza a EdgeMqttGatewayClient con la misma interfaz que usa EdifikaNode:
 * `publish()`, `receiveRemoteCommand()` e `isSessionActive()`.
 *
 *  - Lo que el nodo publica se imprime en el Monitor serie con su topico v1
 *    (`edifika/v1/nodes/{deviceId}/{canal}`) y el gateway simulado reacciona como el real:
 *    adopta el nodo al recibir `announce`, valida las tarjetas de `access` contra una tabla
 *    local, da de alta las de `enrollment` y reenvia `mode` si el heartbeat no coincide.
 *  - Lo que se escribe en el Monitor serie entra como comando de `/commands`: JSON v1 tal cual
 *    o atajos en castellano (escribir `ayuda`).
 *
 * Hilos: `publish()` corre en la tarea de red de Device, `receiveRemoteCommand()` en la de
 * hardware y `pollConsole()` en loop(); el estado compartido se protege con un mutex.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "CommandParser.h"
#include "NodeMessages.h"

class LocalGateway {
private:
    struct CardRecord {
        char uid[24];
        bool granted;
        char message[33];   ///< Texto para la pantalla ("Hola Ana", "Deuda pendiente"...).
    };

    struct PendingCommand {
        bool used;
        unsigned long dueTimestamp;
        char json[200];
    };

    static constexpr uint8_t MAX_CARDS = 10;
    static constexpr uint8_t MAX_PENDING_COMMANDS = 8;
    static constexpr size_t CONSOLE_LINE_SIZE = 200;

    const char* deviceId;
    const unsigned long latencyInMilliseconds;
    const unsigned long grantedLockInMilliseconds;

    SemaphoreHandle_t stateMutex;
    CardRecord cards[MAX_CARDS];
    uint8_t cardCount;
    PendingCommand pending[MAX_PENDING_COMMANDS];
    volatile bool online;
    bool reserved;          ///< Configuracion vigente que el gateway impone al nodo.
    char reservedColor[8];
    uint32_t commandCounter;

    char consoleLine[CONSOLE_LINE_SIZE];
    size_t consoleLength;

    void lock();
    void unlock();
    CardRecord* findCard(const char* uid);   ///< Requiere el mutex tomado.
    void upsertCard(const char* uid, bool granted, const char* message);

    /** @brief Arma un comando v1 y lo deja pendiente (se entrega tras `delayInMilliseconds`). */
    void sendCommand(const char* type, JsonDocument& params, unsigned long delayInMilliseconds);
    void scheduleJson(const char* json, unsigned long delayInMilliseconds);
    void sendCurrentMode(unsigned long delayInMilliseconds);

    void reactToNodeMessage(const char* channel, JsonDocument& body);
    void handleConsoleLine(char* line);
    void printTopic(const char* direction, const char* channel, const char* payload) const;

public:
    LocalGateway(const char* deviceId, unsigned long latencyInMilliseconds, unsigned long grantedLockInMilliseconds);

    /** @brief Crea el mutex, carga las tarjetas de ejemplo y muestra la ayuda. */
    void begin();

    /** @brief Atiende la consola serie. Llamar desde loop(). */
    void pollConsole();

    /** @brief "Publica" un mensaje del nodo. Falso si el gateway simulado esta desconectado. */
    bool publish(const NodeMessage& nodeMessage);

    /** @brief Entrega el siguiente comando cuyo tiempo ya vencio. */
    bool receiveRemoteCommand(NodeCommand& destination);

    /** @brief Equivale a la sesion MQTT activa: se apaga con `offline` para probar el timeout. */
    [[nodiscard]] bool isSessionActive() const;

    static void printHelp();
};

#endif // LOCAL_GATEWAY_H
