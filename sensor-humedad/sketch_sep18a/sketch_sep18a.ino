#include "DHT.h"
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  delay(2000);                        // el DHT11 no acepta lecturas más rápidas
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) { Serial.println("Error de lectura"); return; }
  Serial.printf("Humedad: %.1f %%   Temp: %.1f C\n", h, t);
}