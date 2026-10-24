#ifndef EDGE_MQTT_GATEWAY_CLIENT_H
#define EDGE_MQTT_GATEWAY_CLIENT_H

/**
 * @file EdgeMqttGatewayClient.h
 * @brief Declares the MQTT client that talks to the Edifika Edge Gateway (contract v1).
 * @ingroup gateway_module
 */

#include "../ModestIoT/GatewayClient.h"
#include "../telemetry/NodeMessages.h"
#include <PubSubClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

/**
 * @name Gateway: Protocol-Specific Implementations
 * @ingroup gateway_module
 * @{
 */

/**
 * @brief Command received on `edifika/v1/nodes/{deviceId}/commands`, already parsed.
 */
struct RemoteCommand {
    char commandId[40]; ///< `commandId`, echoed back in the ack.
    char type[24]; ///< `type`, e.g. `"unlock"`.
    uint32_t durationInMilliseconds; ///< `params.duration_ms`, or 0 when absent.
};

/**
 * @brief Bidirectional MQTT gateway for the node.
 * @ingroup gateway_module
 *
 * Extends `GatewayClient` because `MqttGatewayClient` publishes to a single fixed topic and
 * never subscribes. This client publishes every `NodeMessage` on its own channel, subscribes
 * to the commands topic, and hands parsed commands over through a FreeRTOS queue so that the
 * device consumes them on its own task.
 *
 * Threading: `maintainSession()` runs on the Arduino loop task, `publish()` on the `Device`
 * NetworkWorker; a mutex serializes every access to the underlying PubSubClient.
 */
class EdgeMqttGatewayClient : public GatewayClient {
private:
    PubSubClient mqttClient; ///< Underlying MQTT client instance.
    const char* clientIdentifier; ///< Device identifier, also used as MQTT client id.
    SemaphoreHandle_t sessionMutex; ///< Serializes access to `mqttClient`.
    QueueHandle_t remoteCommandQueue; ///< Parsed commands waiting for the device.
    unsigned long lastConnectionAttemptTimestamp; ///< For reconnection back-off.

    static constexpr size_t TOPIC_BUFFER_SIZE = 96; ///< Capacity of a composed topic.
    static constexpr uint16_t PACKET_BUFFER_SIZE = 512; ///< MQTT packet buffer.
    static constexpr uint8_t REMOTE_COMMAND_QUEUE_LENGTH = 4; ///< Pending commands capacity.
    static constexpr unsigned long RECONNECT_INTERVAL_IN_MILLISECONDS = 5000; ///< Back-off.
    static constexpr TickType_t MUTEX_TIMEOUT_TICKS = pdMS_TO_TICKS(1000); ///< Publish wait.

    char commandsTopic[TOPIC_BUFFER_SIZE]; ///< `edifika/v1/nodes/{deviceId}/commands`.

    void composeTopic(char* destination, const char* channel) const;
    void ensureBrokerSessionConnection();
    void onIncomingMessage(const char* topic, const uint8_t* payload, unsigned int length);
    bool publishOnChannel(const char* channel, const TelemetryPackage& telemetryPayload);

public:
    /**
     * @param transportDriver Link-layer driver (Wi-Fi) used for readiness checks and recovery.
     * @param networkClient TCP socket exposed by the driver (`getNetworkSocket()`).
     * @param brokerHost Broker IP or host name.
     * @param brokerPort Broker port (1883 without TLS).
     * @param deviceId Node identifier used in topics and as MQTT client id.
     */
    EdgeMqttGatewayClient(ConnectivityDriver& transportDriver, Client& networkClient,
                          const char* brokerHost, uint16_t brokerPort, const char* deviceId);
    ~EdgeMqttGatewayClient() override;

    /**
     * @brief Keeps Wi-Fi and the broker session alive and processes incoming packets.
     * Call it often (every few milliseconds) from a task other than the hardware worker.
     */
    void maintainSession();

    /**
     * @brief Publishes a node message on `edifika/v1/nodes/{deviceId}/{channel}`.
     * @return `true` if the broker accepted the packet.
     */
    bool publish(const NodeMessage& nodeMessage);

    /**
     * @brief Publishes a generic payload on the `readings` channel.
     */
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) override;

    /**
     * @brief Pops the next pending remote command without blocking.
     * @param destination Filled when a command is available.
     * @return `true` if a command was popped.
     */
    bool receiveRemoteCommand(RemoteCommand& destination);

    /** @brief Reports whether the broker session is currently established. */
    [[nodiscard]] bool isSessionActive();
};

/** @} */ // End of Gateway: Protocol-Specific Implementations

#endif // EDGE_MQTT_GATEWAY_CLIENT_H
