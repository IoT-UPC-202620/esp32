#!/bin/sh
# Muestra en vivo todo lo que publican los nodos (hora del broker, topico y JSON).
# Uso: ./broker/watch.sh            -> todos los topicos edifika/#
#      ./broker/watch.sh readings   -> solo un canal (heartbeat, access, readings, ack, commands)
CHANNEL=${1:-#}
TOPIC="edifika/#"
[ "$CHANNEL" != "#" ] && TOPIC="edifika/v1/nodes/+/$CHANNEL"
exec docker exec -it edifika-mosquitto mosquitto_sub -h localhost -t "$TOPIC" -v -F "%I %t %p"
