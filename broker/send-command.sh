#!/usr/bin/env bash
# Ejemplos de mosquitto_pub para enviar comandos al nodo esp32-acc-01 segun el contrato IoT v1 (§1.2).
# Broker local: localhost:1883 (o contenedor edifika-mosquitto).

DEVICE_ID="${DEVICE_ID:-esp32-acc-01}"
TOPIC="edifika/v1/nodes/${DEVICE_ID}/commands"
BROKER_HOST="${BROKER_HOST:-localhost}"
BROKER_PORT="${BROKER_PORT:-1883}"

send() {
    local payload="$1"
    echo "Sending to $TOPIC:"
    echo "$payload"
    if command -v mosquitto_pub >/dev/null 2>&1; then
        mosquitto_pub -h "$BROKER_HOST" -p "$BROKER_PORT" -t "$TOPIC" -m "$payload"
    elif docker ps 2>/dev/null | grep -q edifika-mosquitto; then
        docker exec edifika-mosquitto mosquitto_pub -h localhost -p 1883 -t "$TOPIC" -m "$payload"
    else
        echo "Note: neither mosquitto_pub nor running edifika-mosquitto container found. Command printed above."
    fi
}

CMD="${1:-all}"

case "$CMD" in
    access_result)
        send '{"v":1,"commandId":"cmd-001","type":"access_result","params":{"result":"GRANTED","reason":"ok","message":"Access granted","buzzer":"granted","name":"Ana Perez","lockMs":5000},"ts":1782000000}'
        ;;
    mode)
        send '{"v":1,"commandId":"cmd-002","type":"mode","params":{"mode":"RESERVED","color":"#00FF88"},"ts":1782000000}'
        ;;
    rgb)
        send '{"v":1,"commandId":"cmd-003","type":"rgb","params":{"color":"#00FF88"},"ts":1782000000}'
        ;;
    unlock)
        send '{"v":1,"commandId":"cmd-004","type":"unlock","params":{"durationMs":5000},"ts":1782000000}'
        ;;
    display)
        send '{"v":1,"commandId":"cmd-005","type":"display","params":{"message":"Bienvenido a Edifika"},"ts":1782000000}'
        ;;
    buzzer)
        send '{"v":1,"commandId":"cmd-006","type":"buzzer","params":{"pattern":"granted"},"ts":1782000000}'
        ;;
    alert)
        send '{"v":1,"commandId":"cmd-007","type":"alert","params":{"kind":"smoke","active":true,"message":"Alerta de humo","buzzer":"alert"},"ts":1782000000}'
        ;;
    maintenance)
        send '{"v":1,"commandId":"cmd-008","type":"maintenance","params":{"active":true,"message":"En mantenimiento"},"ts":1782000000}'
        ;;
    time_sync)
        send '{"v":1,"commandId":"cmd-009","type":"time_sync","params":{"epoch":1782000000,"timezone":"America/Lima","driftSeconds":0.5},"ts":1782000000}'
        ;;
    *)
        echo "Uso: $0 [access_result|mode|rgb|unlock|display|buzzer|alert|maintenance|time_sync]"
        echo ""
        echo "Ejemplos mosquitto_pub para cada tipo de comando (§1.2):"
        echo "1. access_result:"
        echo 'mosquitto_pub -h localhost -t "'"$TOPIC"'" -m '\''{"v":1,"commandId":"cmd-001","type":"access_result","params":{"result":"GRANTED","reason":"ok","message":"Access granted","buzzer":"granted","name":"Ana Perez","lockMs":5000},"ts":1782000000}'\'
        echo "2. mode:"
        echo 'mosquitto_pub -h localhost -t "'"$TOPIC"'" -m '\''{"v":1,"commandId":"cmd-002","type":"mode","params":{"mode":"RESERVED","color":"#00FF88"},"ts":1782000000}'\'
        echo "3. rgb:"
        echo 'mosquitto_pub -h localhost -t "'"$TOPIC"'" -m '\''{"v":1,"commandId":"cmd-003","type":"rgb","params":{"color":"#00FF88"},"ts":1782000000}'\'
        echo "4. unlock:"
        echo 'mosquitto_pub -h localhost -t "'"$TOPIC"'" -m '\''{"v":1,"commandId":"cmd-004","type":"unlock","params":{"durationMs":5000},"ts":1782000000}'\'
        echo "5. display:"
        echo 'mosquitto_pub -h localhost -t "'"$TOPIC"'" -m '\''{"v":1,"commandId":"cmd-005","type":"display","params":{"message":"Bienvenido a Edifika"},"ts":1782000000}'\'
        echo "6. buzzer:"
        echo 'mosquitto_pub -h localhost -t "'"$TOPIC"'" -m '\''{"v":1,"commandId":"cmd-006","type":"buzzer","params":{"pattern":"granted"},"ts":1782000000}'\'
        echo "7. alert:"
        echo 'mosquitto_pub -h localhost -t "'"$TOPIC"'" -m '\''{"v":1,"commandId":"cmd-007","type":"alert","params":{"kind":"smoke","active":true,"message":"Alerta de humo","buzzer":"alert"},"ts":1782000000}'\'
        echo "8. maintenance:"
        echo 'mosquitto_pub -h localhost -t "'"$TOPIC"'" -m '\''{"v":1,"commandId":"cmd-008","type":"maintenance","params":{"active":true,"message":"En mantenimiento"},"ts":1782000000}'\'
        echo "9. time_sync:"
        echo 'mosquitto_pub -h localhost -t "'"$TOPIC"'" -m '\''{"v":1,"commandId":"cmd-009","type":"time_sync","params":{"epoch":1782000000,"timezone":"America/Lima","driftSeconds":0.5},"ts":1782000000}'\'
        ;;
esac
