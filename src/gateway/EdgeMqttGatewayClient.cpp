/**
 * @file EdgeMqttGatewayClient.cpp
 * @brief Implements topic routing, session upkeep and command intake for the edge MQTT link.
 */

#include "EdgeMqttGatewayClient.h"
#include <ArduinoJson.h>

namespace {
constexpr const char* TOPIC_PREFIX = "edifika/v1/nodes/";
}

EdgeMqttGatewayClient::EdgeMqttGatewayClient(ConnectivityDriver& transportDriver, Client& networkClient,
                                             const char* brokerHost, uint16_t brokerPort, const char* deviceId)
    : GatewayClient(transportDriver, brokerHost),
      mqttClient(networkClient),
      clientIdentifier(deviceId),
      sessionMutex(xSemaphoreCreateMutex()),
      remoteCommandQueue(xQueueCreate(REMOTE_COMMAND_QUEUE_LENGTH, sizeof(NodeCommand))),
      lastConnectionAttemptTimestamp(0),
      commandsTopic("")
{
    composeTopic(commandsTopic, "commands");
    mqttClient.setServer(brokerHost, brokerPort);
    mqttClient.setBufferSize(PACKET_BUFFER_SIZE);
    mqttClient.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
        onIncomingMessage(topic, payload, length);
    });
}

EdgeMqttGatewayClient::~EdgeMqttGatewayClient() {
    if (remoteCommandQueue != nullptr) vQueueDelete(remoteCommandQueue);
    if (sessionMutex != nullptr) vSemaphoreDelete(sessionMutex);
}

void EdgeMqttGatewayClient::composeTopic(char* destination, const char* channel) const {
    snprintf(destination, TOPIC_BUFFER_SIZE, "%s%s/%s", TOPIC_PREFIX, clientIdentifier, channel);
}

void EdgeMqttGatewayClient::maintainSession() {
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect();   // asynchronous, returns immediately
        return;
    }

    if (xSemaphoreTake(sessionMutex, portMAX_DELAY) != pdTRUE) return;
    ensureBrokerSessionConnection();
    if (mqttClient.connected()) {
        mqttClient.loop();   // keep-alive pings and incoming commands (callback runs here)
    }
    xSemaphoreGive(sessionMutex);
}

// Called with the mutex held
void EdgeMqttGatewayClient::ensureBrokerSessionConnection() {
    if (mqttClient.connected()) return;

    unsigned long now = millis();
    if (lastConnectionAttemptTimestamp != 0 && now - lastConnectionAttemptTimestamp < RECONNECT_INTERVAL_IN_MILLISECONDS) {
        return;
    }
    lastConnectionAttemptTimestamp = now;

    if (mqttClient.connect(clientIdentifier)) {
        mqttClient.subscribe(commandsTopic, 1);
        Serial.printf("MQTT conectado, escuchando %s\n", commandsTopic);
    } else {
        Serial.printf("MQTT sin conexion (estado %d), reintento en %lu ms\n",
                      mqttClient.state(), RECONNECT_INTERVAL_IN_MILLISECONDS);
    }
}

bool EdgeMqttGatewayClient::publish(const NodeMessage& nodeMessage) {
    return publishOnChannel(nodeMessage.getChannel(), nodeMessage);
}

bool EdgeMqttGatewayClient::sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
    return publishOnChannel("readings", abstractTelemetryPayload);
}

bool EdgeMqttGatewayClient::publishOnChannel(const char* channel, const TelemetryPackage& telemetryPayload) {
    if (!communicationTransportDriver.canTransmit()) return false;

    char topic[TOPIC_BUFFER_SIZE];
    composeTopic(topic, channel);
    String serializedPayloadOutput = telemetryPayload.serializeAsString();

    if (xSemaphoreTake(sessionMutex, MUTEX_TIMEOUT_TICKS) != pdTRUE) return false;
    bool publishSuccessStatus = mqttClient.connected() && mqttClient.publish(topic, serializedPayloadOutput.c_str());
    xSemaphoreGive(sessionMutex);
    return publishSuccessStatus;
}

// Runs inside mqttClient.loop(), i.e. on the loop task with the mutex held
void EdgeMqttGatewayClient::onIncomingMessage(const char* topic, const uint8_t* payload, unsigned int length) {
    if (strcmp(topic, commandsTopic) != 0) return;

    NodeCommand remoteCommand = CommandParser::parse(payload, length);
    if (xQueueSend(remoteCommandQueue, &remoteCommand, 0) != pdPASS) {
        Serial.println("Comando descartado: cola llena");
    }
}

bool EdgeMqttGatewayClient::receiveRemoteCommand(NodeCommand& destination) {
    return xQueueReceive(remoteCommandQueue, &destination, 0) == pdPASS;
}

bool EdgeMqttGatewayClient::isSessionActive() {
    if (xSemaphoreTake(sessionMutex, 0) != pdTRUE) return false;   // busy means a session op is running
    bool active = mqttClient.connected();
    xSemaphoreGive(sessionMutex);
    return active;
}
