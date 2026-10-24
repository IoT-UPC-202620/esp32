# Broker MQTT para el nodo ESP32

Este directorio proporciona el entorno MQTT para pruebas y desarrollo local del nodo ESP32.

## 1. Broker de desarrollo local (este directorio)

Usa la imagen oficial `eclipse-mosquitto:2` en el puerto 1883, sin TLS ni autenticación, para pruebas directas del firmware sin levantar la plataforma completa:

```bash
# Levantar el broker local y el logger
cd broker
docker compose up -d

# Ver los mensajes publicados por el nodo en tiempo real
./watch.sh               # Todos los canales
./watch.sh access        # Solo intentos de acceso
./watch.sh readings      # Solo telemetria de sensores
./watch.sh ack           # Solo confirmaciones de comandos

# Enviar comandos de prueba al nodo
./send-command.sh unlock
./send-command.sh access_result
./send-command.sh mode
./send-command.sh rgb
./send-command.sh display
./send-command.sh buzzer
./send-command.sh alert
./send-command.sh maintenance
./send-command.sh time_sync
```

## 2. Broker de integración (Edifika-IoT-Platform)

En un entorno de integración o producción, el broker MQTT corre como parte del stack de `Edifika-IoT-Platform` (`docker-compose.yml`), donde:
- El **Edge Gateway** (`edge-gateway`) se suscribe a los eventos del nodo y publica comandos hacia él.
- El gateway puentea los eventos hacia **RabbitMQ** (`edifika.events`) y sincroniza credenciales y reservas con **IoT Access Management**.
- Para conectar el nodo a la plataforma, configure en `src/config/Secrets.h` la IP del host donde corre la plataforma y el puerto `1883`.
