/**
 * @file LocalGateway.cpp
 * @brief Implementa el Edge Gateway simulado y la consola de comandos por el Monitor serie.
 */

#include "LocalGateway.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace {

// Separa "verbo resto": devuelve el resto (sin espacios iniciales) y termina el verbo en '\0'
char* splitVerb(char* line) {
    char* rest = line;
    while (*rest != '\0' && *rest != ' ') {
        *rest = static_cast<char>(tolower(static_cast<unsigned char>(*rest)));
        rest++;
    }
    if (*rest == ' ') {
        *rest++ = '\0';
        while (*rest == ' ') rest++;
    }
    return rest;
}

void normalizeColor(const char* text, char* destination) {
    // Acepta "#00FF88" o "00FF88"
    if (text[0] == '#') snprintf(destination, 8, "%s", text);
    else snprintf(destination, 8, "#%s", text);
}

} // namespace

LocalGateway::LocalGateway(const char* deviceId, unsigned long latencyInMilliseconds,
                           unsigned long grantedLockInMilliseconds)
    : deviceId(deviceId),
      latencyInMilliseconds(latencyInMilliseconds),
      grantedLockInMilliseconds(grantedLockInMilliseconds),
      stateMutex(nullptr),
      cards{},
      cardCount(0),
      pending{},
      online(true),
      reserved(false),
      reservedColor("#00FF88"),
      commandCounter(0),
      consoleLine(""),
      consoleLength(0) {}

void LocalGateway::begin() {
    stateMutex = xSemaphoreCreateMutex();
    // UIDs de las tarjetas del lector MFRC522 de Wokwi (selector "Card" del lector)
    upsertCard("11223344", true, "Hola Ana (Dpto 101)");    // Green Card
    upsertCard("01020304", true, "Hola Luis (Dpto 202)");   // Blue Card
    upsertCard("C0FFEE99", true, "Conserje");               // Key Fob
    upsertCard("AABBCCDD", false, "Deuda pendiente");       // Red Card
    // Yellow Card (55667788) y NFC Tag (04112233445566) no estan registradas
    printHelp();
}

void LocalGateway::lock() {
    if (stateMutex != nullptr) xSemaphoreTake(stateMutex, portMAX_DELAY);
}

void LocalGateway::unlock() {
    if (stateMutex != nullptr) xSemaphoreGive(stateMutex);
}

bool LocalGateway::isSessionActive() const { return online; }

// ---------------- tarjetas ----------------
LocalGateway::CardRecord* LocalGateway::findCard(const char* uid) {
    for (uint8_t i = 0; i < cardCount; ++i) {
        if (strcasecmp(cards[i].uid, uid) == 0) return &cards[i];
    }
    return nullptr;
}

void LocalGateway::upsertCard(const char* uid, bool granted, const char* message) {
    lock();
    CardRecord* card = findCard(uid);
    if (card == nullptr && cardCount < MAX_CARDS) card = &cards[cardCount++];
    if (card != nullptr) {
        snprintf(card->uid, sizeof(card->uid), "%s", uid);
        card->granted = granted;
        snprintf(card->message, sizeof(card->message), "%s", message);
    }
    unlock();
}

// ---------------- comandos hacia el nodo ----------------
void LocalGateway::scheduleJson(const char* json, unsigned long delayInMilliseconds) {
    lock();
    PendingCommand* slot = nullptr;
    for (uint8_t i = 0; i < MAX_PENDING_COMMANDS; ++i) {
        if (!pending[i].used) {
            slot = &pending[i];
            break;
        }
    }
    if (slot != nullptr) {
        slot->used = true;
        slot->dueTimestamp = millis() + delayInMilliseconds;
        snprintf(slot->json, sizeof(slot->json), "%s", json);
    }
    unlock();
    if (slot == nullptr) Serial.println("[GW] Cola de comandos llena: comando descartado");
}

void LocalGateway::sendCommand(const char* type, JsonDocument& params, unsigned long delayInMilliseconds) {
    JsonDocument command;
    char commandId[24];
    snprintf(commandId, sizeof(commandId), "gw-%lu", static_cast<unsigned long>(++commandCounter));
    command["commandId"] = commandId;
    command["type"] = type;
    command["params"] = params;
    char json[200];
    serializeJson(command, json, sizeof(json));
    scheduleJson(json, delayInMilliseconds);
}

void LocalGateway::sendCurrentMode(unsigned long delayInMilliseconds) {
    JsonDocument params;
    params["mode"] = reserved ? "RESERVED" : "FREE_ACCESS";
    if (reserved) params["color"] = reservedColor;
    sendCommand("mode", params, delayInMilliseconds);
}

bool LocalGateway::receiveRemoteCommand(NodeCommand& destination) {
    if (!online) return false;   // sin "broker" los comandos esperan
    char json[200];
    bool found = false;
    unsigned long now = millis();
    lock();
    for (uint8_t i = 0; i < MAX_PENDING_COMMANDS; ++i) {
        if (pending[i].used && static_cast<long>(now - pending[i].dueTimestamp) >= 0) {
            memcpy(json, pending[i].json, sizeof(json));
            pending[i].used = false;
            found = true;
            break;
        }
    }
    unlock();
    if (!found) return false;
    printTopic("<-", "commands", json);
    destination = CommandParser::parse(reinterpret_cast<const uint8_t*>(json), strlen(json));
    return true;
}

// ---------------- mensajes del nodo ----------------
void LocalGateway::printTopic(const char* direction, const char* channel, const char* payload) const {
    Serial.printf("[MQTT %s] edifika/v1/nodes/%s/%s %s\n", direction, deviceId, channel, payload);
}

bool LocalGateway::publish(const NodeMessage& nodeMessage) {
    if (!online) return false;
    JsonDocument body;
    nodeMessage.serialize(body);
    String payload;
    serializeJson(body, payload);
    printTopic("->", nodeMessage.getChannel(), payload.c_str());
    reactToNodeMessage(nodeMessage.getChannel(), body);
    return true;
}

// Lo que haria el Edge Gateway real con cada canal
void LocalGateway::reactToNodeMessage(const char* channel, JsonDocument& body) {
    if (strcmp(channel, "announce") == 0) {
        // Nodo conocido: el gateway lo adopta enviando la configuracion vigente
        Serial.println("[GW] Nodo anunciado: lo adopto con el modo vigente");
        sendCurrentMode(latencyInMilliseconds);
        return;
    }

    if (strcmp(channel, "access") == 0) {
        const char* credential = body["credential"] | "";
        lock();
        CardRecord* card = findCard(credential);
        bool granted = card != nullptr && card->granted;
        char message[33];
        snprintf(message, sizeof(message), "%s", card != nullptr ? card->message : "Tarjeta no registrada");
        unlock();

        Serial.printf("[GW] Tarjeta %s -> %s (%s)\n", credential, granted ? "GRANTED" : "DENIED", message);
        JsonDocument params;
        params["result"] = granted ? "GRANTED" : "DENIED";
        params["message"] = message;
        if (granted) params["lockMs"] = grantedLockInMilliseconds;
        sendCommand("access_result", params, latencyInMilliseconds);
        return;
    }

    if (strcmp(channel, "enrollment") == 0) {
        const char* credential = body["credential"] | "";
        upsertCard(credential, true, "Tarjeta nueva");
        Serial.printf("[GW] Tarjeta %s registrada con acceso concedido\n", credential);
        return;
    }

    if (strcmp(channel, "heartbeat") == 0) {
        // Resincroniza si lo que muestra la puerta no coincide con la configuracion vigente
        const char* mode = body["mode"] | "";
        const char* color = body["color"] | "";
        bool nodeReserved = strcmp(mode, "RESERVED") == 0;
        bool stale = nodeReserved != reserved || (reserved && strcasecmp(color, reservedColor) != 0);
        if (stale) {
            Serial.println("[GW] El heartbeat no coincide con la configuracion: reenvio mode");
            sendCurrentMode(latencyInMilliseconds);
        }
        return;
    }
    // readings y ack: solo se registran en el log
}

// ---------------- consola serie ----------------
void LocalGateway::pollConsole() {
    while (Serial.available() > 0) {
        char c = static_cast<char>(Serial.read());
        if (c == '\r' || c == '\n') {
            if (consoleLength > 0) {
                consoleLine[consoleLength] = '\0';
                consoleLength = 0;
                handleConsoleLine(consoleLine);
            }
        } else if (consoleLength < CONSOLE_LINE_SIZE - 1) {
            consoleLine[consoleLength++] = c;
        }
    }
}

void LocalGateway::handleConsoleLine(char* line) {
    while (*line == ' ') line++;
    if (*line == '\0') return;

    // JSON v1 tal cual, como si llegara por edifika/v1/nodes/{id}/commands
    if (*line == '{') {
        scheduleJson(line, 0);
        return;
    }

    char* rest = splitVerb(line);
    JsonDocument params;

    if (strcmp(line, "ayuda") == 0 || strcmp(line, "help") == 0) {
        printHelp();
    } else if (strcmp(line, "reservar") == 0) {
        if (*rest != '\0') {
            char color[8];
            normalizeColor(rest, color);
            RgbColor parsed;
            if (!CommandParser::parseHexColor(color, parsed)) {
                Serial.println("[GW] Color invalido, usa #RRGGBB");
                return;
            }
            snprintf(reservedColor, sizeof(reservedColor), "%s", color);
        }
        reserved = true;
        sendCurrentMode(0);
    } else if (strcmp(line, "libre") == 0) {
        reserved = false;
        sendCurrentMode(0);
    } else if (strcmp(line, "color") == 0) {
        char color[8];
        normalizeColor(rest, color);
        if (reserved) snprintf(reservedColor, sizeof(reservedColor), "%s", color);
        params["color"] = color;
        sendCommand("rgb", params, 0);
    } else if (strcmp(line, "mant") == 0) {
        char* message = splitVerb(rest);   // "on mensaje..." / "off"
        params["active"] = strcmp(rest, "off") != 0;
        if (*message != '\0') params["message"] = message;
        sendCommand("maintenance", params, 0);
    } else if (strcmp(line, "abrir") == 0) {
        long durationInMilliseconds = atol(rest);
        if (durationInMilliseconds > 0) params["durationMs"] = durationInMilliseconds;
        sendCommand("unlock", params, 0);
    } else if (strcmp(line, "msg") == 0) {
        params["message"] = rest;
        sendCommand("display", params, 0);
    } else if (strcmp(line, "alerta") == 0) {
        params["active"] = true;
        params["message"] = *rest != '\0' ? rest : "ALERTA";
        sendCommand("alert", params, 0);
    } else if (strcmp(line, "buzzer") == 0) {
        params["pattern"] = *rest != '\0' ? rest : "granted";
        sendCommand("buzzer", params, 0);
    } else if (strcmp(line, "registro") == 0) {
        params["active"] = strcmp(rest, "off") != 0;
        params["seconds"] = 60;
        sendCommand("enroll", params, 0);
    } else if (strcmp(line, "identificar") == 0) {
        params["seconds"] = 5;
        sendCommand("identify", params, 0);
    } else if (strcmp(line, "offline") == 0) {
        online = false;
        Serial.println("[GW] Gateway DESCONECTADO: las tarjetas venceran a los 5 s (Gateway no disponible)");
    } else if (strcmp(line, "online") == 0) {
        online = true;
        Serial.println("[GW] Gateway CONECTADO");
    } else if (strcmp(line, "tarjetas") == 0) {
        lock();
        Serial.printf("[GW] %u tarjetas registradas:\n", cardCount);
        for (uint8_t i = 0; i < cardCount; ++i) {
            Serial.printf("  %s  %-7s  %s\n", cards[i].uid, cards[i].granted ? "GRANTED" : "DENIED", cards[i].message);
        }
        unlock();
    } else {
        Serial.printf("[GW] Comando desconocido: %s (escribe 'ayuda')\n", line);
    }
}

void LocalGateway::printHelp() {
    Serial.println();
    Serial.println("=== Gateway local simulado (sin backend) ===");
    Serial.println("RFID: clic en el lector, elige la tarjeta y pulsa TAP");
    Serial.println("  Green/Blue/Key Fob = concedido, Red = moroso, Yellow/NFC = no registradas");
    Serial.println("Comandos por el Monitor serie:");
    Serial.println("  reservar [#RRGGBB]   puerta RESERVED con ese color (por defecto #00FF88)");
    Serial.println("  libre                puerta FREE_ACCESS");
    Serial.println("  color #RRGGBB        cambia el color del LED (solo en RESERVED)");
    Serial.println("  mant on [mensaje]    mantenimiento / mant off");
    Serial.println("  abrir [ms]           abre la cerradura");
    Serial.println("  msg <texto>          mensaje temporal en la pantalla");
    Serial.println("  alerta [texto]       alerta con sonido");
    Serial.println("  buzzer granted|denied|alert");
    Serial.println("  registro [off]       modo registro: la siguiente tarjeta se da de alta");
    Serial.println("  identificar          parpadeo blanco + beep");
    Serial.println("  offline | online     simula la caida del gateway");
    Serial.println("  tarjetas             lista las tarjetas registradas");
    Serial.println("  {json}               comando v1 tal cual, ej: {\"type\":\"unlock\",\"params\":{\"durationMs\":2000}}");
    Serial.println();
}
