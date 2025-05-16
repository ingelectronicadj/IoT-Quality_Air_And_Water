// Proyecto de Monitoreo de Calidad del Agua y Aire con NodeMCU ESP8266
// Autor: Diego Javier Mena Amado
// Email: ingelectronicadj@gmail.com
// Licencia: GNU General Public License v3.0

#define BLYNK_TEMPLATE_ID "TMPL2ieIP4YG3"
#define BLYNK_TEMPLATE_NAME "Quality Water"
#define BLYNK_AUTH_TOKEN "EVXWMm138Ebxv07m-b3_FD74GFAN3gP2"
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <EEPROM.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include "Adafruit_SGP30.h"

#define DHTPIN 2
#define DHTTYPE DHT11
#define TdsSensorPin A0

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SGP30 sgp;
WidgetTerminal terminal(V3); // Terminal de Blynk en el pin V3

// Variables globales
float temperature = 25.0;
float humidity = 0.0;
float tdsValue = 0.0;
float ecValue = 0.0;
uint16_t TVOC = 0;
uint16_t eCO2 = 0;
uint32_t absHumidity = 0;

// Constantes
const float VREF = 3.28;
const int ADC_RES = 1024;
const float TDS_FACTOR = 0.5;
const float kValue = 1.0;

// WiFi
char ssid[] = "WEALTH 2.4G Y 5G";
char pass[] = "Nomelase**";

// ---------------- FUNCIONES ----------------

uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  const float absH = 216.7f * ((humidity / 100.0f) * 6.112f *
    exp((17.62f * temperature) / (243.12f + temperature)) /
    (273.15f + temperature));
  return static_cast<uint32_t>(1000.0f * absH);
}

// Función para conectar a WiFi y manejar reintentos
void connectToWiFi() {
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  int wifiRetries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetries < 20) {
    delay(500);
    wifiRetries++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    terminal.println("Error de conexión WiFi. Reiniciando en 2 minutos...");
    terminal.flush();
    ESP.deepSleep(120e6);
  }
  terminal.println("Dispositivo conectado a WiFi...");
}

void readDHTSensor() {
  temperature = dht.readTemperature();
  humidity    = dht.readHumidity();
  if (!isnan(temperature) && !isnan(humidity)) {
    absHumidity = getAbsoluteHumidity(temperature, humidity);
    sgp.setHumidity(absHumidity);
  } else {
    terminal.println("Error al leer del sensor DHT");
  }
}

void getAverageSGP30(uint16_t &avgECO2, uint16_t &avgTVOC, int numReadings = 5) {
  uint32_t sumECO2 = 0, sumTVOC = 0;
  int validReadings = 0;
  int readingsDone = 0;
  unsigned long lastMillis = millis();

  while (readingsDone < numReadings) {
    if (millis() - lastMillis >= 500) {        // cada 500 ms
      lastMillis = millis();
      readDHTSensor();                         // Refrescar compensación
      if (sgp.IAQmeasure()) {
        if (sgp.eCO2 > 400 && sgp.TVOC > 10) {
          sumECO2  += sgp.eCO2;
          sumTVOC += sgp.TVOC;
          validReadings++;
        }
      }
      readingsDone++;
    }
    // aquí podríamos hacer otras tareas sin bloquear
  }

  if (validReadings > 0) {
    avgECO2 = sumECO2 / validReadings;
    avgTVOC = sumTVOC / validReadings;
  } else {
    avgECO2 = sgp.eCO2;
    avgTVOC = sgp.TVOC;
  }
}

void readTDS() {
  terminal.println("Sensor TDS tomando lecturas...");
  int analogSum = 0;
  int numReadings = 20;
  int readingsDone = 0;
  unsigned long lastMillis = millis();

  while (readingsDone < numReadings) {
    if (millis() - lastMillis >= 10) {        // cada 10 ms
      lastMillis = millis();
      analogSum  += analogRead(TdsSensorPin);
      readingsDone++;
    }
    // otras tareas...
  }

  float avgAnalog = analogSum / float(numReadings);
  float voltage   = avgAnalog * (VREF / ADC_RES);
  ecValue = (133.42 * voltage * voltage * voltage
           - 255.86 * voltage * voltage
           + 857.39 * voltage) * kValue;
  float compCoeff = 1.0 + 0.02 * (temperature - 25.0);
  float compEC    = ecValue / compCoeff;
  tdsValue        = compEC * TDS_FACTOR;

  terminal.print("EC: "); terminal.print(compEC, 2);
  terminal.print(" µS/cm | TDS: "); terminal.print(tdsValue, 0); terminal.println(" ppm");

  // Clasificación de la calidad del agua
  terminal.print("Calidad del Agua: ");
  if (tdsValue < 300)       terminal.println("EXCELENTE");
  else if (tdsValue < 600)  terminal.println("BUENA");
  else if (tdsValue < 900)  terminal.println("REGULAR");
  else if (tdsValue < 1200) terminal.println("POBRE");
  else                      terminal.println("INACEPTABLE");
}

void initializeSGP30() {
  if (!sgp.begin()) {
    terminal.println("Sensor SGP30 no detectado :(");
    return;
  }
  terminal.println("Sensor SGP30 precalentando. Esperando condiciones válidas...");

  bool readyToMeasure   = false;
  unsigned long start   = millis();

  while (!readyToMeasure && millis() - start < 60000) {
    if (sgp.IAQmeasure() && sgp.eCO2 > 400 && sgp.TVOC > 10) {
      readyToMeasure = true;
      eCO2           = sgp.eCO2;
      TVOC           = sgp.TVOC;
    }
    delay(1000);
  }

  if (readyToMeasure) {
    readDHTSensor();
    getAverageSGP30(eCO2, TVOC);
    terminal.println("SGP30 listo para medir.");
  } else {
    terminal.println("SGP30 no alcanzó condiciones válidas. Usando última lectura.");
    sgp.IAQmeasure();
    eCO2 = sgp.eCO2;
    TVOC = sgp.TVOC;
  }

  // Muestra los valores medidos al final de la inicialización
  terminal.print("eCO2: "); terminal.print(eCO2);
  terminal.print(" ppm | TVOC: "); terminal.print(TVOC);
  terminal.println(" ppb");

  // Clasificación de la calidad del aire
  terminal.print("Calidad del Aire: ");
  if (eCO2 < 600)      terminal.println("EXCELENTE");
  else if (eCO2 < 800)  terminal.println("BUENO");
  else if (eCO2 < 1000) terminal.println("REGULAR");
  else if (eCO2 < 1500) terminal.println("POBRE");
  else                  terminal.println("INACEPTABLE");
}

void sendDataToBlynk() {
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, tdsValue);
  Blynk.virtualWrite(V4, ecValue);
  Blynk.virtualWrite(V5, TVOC);
  Blynk.virtualWrite(V6, eCO2);
  Blynk.virtualWrite(V7, absHumidity);
}

// ---------------- SETUP ----------------

void setup() {
  Serial.begin(9600);
  dht.begin();

  connectToWiFi();       // Maneja conexión WiFi y reintentos

  initializeSGP30();     // Lectura, compensación y promedio SGP30
  readDHTSensor();       // Lectura inicial DHT
  readTDS();             // Lectura, compensación y promedio TDS

  sendDataToBlynk();     // Envía todas las lecturas a Blynk

  terminal.println("Entrando en deepSleep 2 min...");
  terminal.flush();

  // Espera no bloqueante de 2 segundos
  unsigned long waitStart = millis();
  while (millis() - waitStart < 2000) {
    delay(100); // Espera 100 milisegundos para evitar bucle infinito
  }

  ESP.deepSleep(120e6);
}

void loop() {
  // No usado con deepSleep
}
