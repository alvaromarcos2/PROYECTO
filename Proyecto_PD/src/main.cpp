/*#include <Arduino.h>
#include "DHT.h"

// --- Configuración Sensor DHT11 ---
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- Configuración Sensor MQ2 ---
const int mq2pin = 12;

// --- Lógica de Máquina de Estados para lecturas alternadas ---

// 1. Definimos los posibles estados: leer un sensor o el otro.
enum SensorState {
  STATE_READ_DHT,
  STATE_READ_MQ2
};

// 2. Creamos una variable para guardar el estado actual. Empezamos por el DHT.
SensorState currentState = STATE_READ_DHT;

// 3. Usamos un solo temporizador para controlar el intervalo entre CUALQUIER lectura.
unsigned long previousMillis = 0;
const long interval = 1000; // Leeremos un sensor cada 1000 ms (1 segundo)


// --- Funciones de lectura (sin cambios) ---

void leerSensorDHT() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Error al leer el sensor DHT11");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.print(" °C / ");
  Serial.print("Humedad: ");
  Serial.print(humidity);
  Serial.println(" %");
}

void leerSensorMQ2() {
  int mq2value = analogRead(mq2pin);
  float voltage = (mq2value / 4095.0) * 3.3;

  Serial.print("MQ2 ADC: ");
  Serial.print(mq2value);
  Serial.print(" / Voltaje: ");
  Serial.print(voltage, 2);
  Serial.print(" V");

  if (mq2value > 600) {
    Serial.println(" --> ¡Alerta de gas o humo detectado!");
  } else {
    Serial.println();
  }
}


// --- Setup unificado (sin cambios) ---
void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando sistema de sensores (modo alternado)...");
  dht.begin();
  analogReadResolution(12);
}


// --- Loop principal con Máquina de Estados ---
void loop() {
  unsigned long currentMillis = millis();

  // Comprobamos si ha pasado el tiempo para la SIGUIENTE lectura
  if (currentMillis - previousMillis >= interval) {
    // Guardamos el tiempo de esta ejecución
    previousMillis = currentMillis;

    // Decidimos qué hacer según el estado actual
    if (currentState == STATE_READ_DHT) {
      // Es el turno del DHT
      leerSensorDHT();
      // Para la próxima vez, cambiamos el estado al MQ2
      currentState = STATE_READ_MQ2;
    } 
    else if (currentState == STATE_READ_MQ2) {
      // Es el turno del MQ2
      leerSensorMQ2();
      // Para la próxima vez, volvemos al estado del DHT
      currentState = STATE_READ_DHT;
    }
  }
  // AÑADIR WEB 
  // AÑADIR EL READ DE PRUEBA
}
*/
#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>

// --- Configuración WiFi ---
const char* ssid = "iPhone de Alvaro";
const char* password = "AlvaroMarcos";

// --- Pines ---
#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WiFiServer server(80);

// Umbral de temperatura
const float TEMP_MAXIMA = 30.0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Conexión WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. Dirección IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("Cliente conectado");

    while (!client.available()) {
      delay(1);
    }

    String req = client.readStringUntil('\r');
    Serial.println("Petición: " + req);
    client.flush();

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    // Verifica si hay error en lectura
    if (isnan(temp) || isnan(hum)) {
      temp = -1;
      hum = -1;
    }

    // Determinar si se excede la temperatura
    String bgColor = (temp > TEMP_MAXIMA) ? "red" : "white";

    // Página HTML dinámica
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Sensor DHT11</title></head>";
    html += "<body style='background-color:" + bgColor + "; font-family:Arial;'>";
    html += "<h1>Lecturas del sensor DHT11</h1>";
    html += "<p><strong>Temperatura:</strong> " + String(temp) + " °C</p>";
    html += "<p><strong>Humedad:</strong> " + String(hum) + " %</p>";
    html += "<form action='/prueba'><button style='padding:10px;font-size:16px;'>Prueba</button></form>";
    html += "</body></html>";

    // Enviar respuesta HTTP
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.println(html);
    client.println();

    delay(1);
    Serial.println("Cliente desconectado.");
  }
}