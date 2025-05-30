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
float humidity    = 0.0;
float tdsValue    = 0.0;
float ecValue     = 0.0;
uint16_t TVOC     = 0;
uint16_t eCO2     = 0;
uint32_t absHumidityQ16 = 0;  // Humedad absoluta en formato Q16.16 para compensación SGP30
uint32_t absH_mgm3    = 0;    // Humedad absoluta en mg/m³ (para cálculo)
float    absH_gm3     = 0.0f; // Humedad absoluta en g/m³ (para visualización y envío)

// Constantes
const float VREF      = 3.28;
const int   ADC_RES   = 1024;
const float TDS_FACTOR= 0.5;
const float kValue    = 1.0;

// WiFi
char ssid[] = "Mi casa";
char pass[] = "363103631";

// ---------------- FUNCIONES ----------------

// Calcula humedad absoluta en mg/m³
uint32_t getAbsoluteHumidity_mgm3(float temperature, float humidity) {
  float absH = 216.7f * ((humidity / 100.0f) * 6.112f *
    exp((17.62f * temperature) / (243.12f + temperature)) /
    (273.15f + temperature));
  return static_cast<uint32_t>(absH * 1000.0f); // Conversión a mg/m³
}

// Conexión WiFi con reintentos
void connectToWiFi() {
  Serial.println("Intentando conectar a WiFi...");
  Serial.print("SSID: "); Serial.println(ssid);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  int wifiRetries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetries < 20) {
    Serial.print(".");
    delay(500);
    wifiRetries++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nError de conexión WiFi. Reiniciando en 2 minutos...");
    terminal.println("Error de conexión WiFi. Reiniciando en 2 minutos...");
    terminal.flush();
    ESP.deepSleep(120e6);  // 2 minutos
  } else {
    Serial.println("\nConectado a WiFi.");
    Serial.print("Dirección IP: "); Serial.println(WiFi.localIP());

    terminal.println("Dispositivo conectado a WiFi...");
    terminal.print("IP: "); terminal.println(WiFi.localIP());
  }
}

// Lee DHT11 y actualiza compensación del SGP30
void readDHTSensor() {
  // Intento inicial sin delay
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  // Si no hay dato válido (>0) o NaN, espera 2s y reintenta una vez
  if (isnan(temp) || isnan(hum) || temp <= 0.0f || hum <= 0.0f) {
    terminal.println("Lectura DHT inválida, esperando 2s antes de reintentar...");
    delay(2000);
    temp = dht.readTemperature();
    hum  = dht.readHumidity();
  }

  if (!isnan(temp) && !isnan(hum) && temp > 0.0f && hum > 0.0f) {
    temperature = temp;
    humidity    = hum;

    // Calcula humedad absoluta
    absH_mgm3 = getAbsoluteHumidity_mgm3(temperature, humidity);
    absH_gm3  = absH_mgm3 / 1000.0f;  // Conversión a g/m³ para visualización

    // Muestra humedad absoluta en terminal
    terminal.print("Humedad abs: ");
    terminal.print(absH_gm3, 2);
    terminal.println(" g/m3");

    // Prepara valor Q16.16 para el SGP30
    absHumidityQ16 = absH_mgm3 * 65536ul;
    sgp.setHumidity(absHumidityQ16);
  } else {
    terminal.println("Error al leer del sensor DHT tras reintento");
  }
}

// Promedia lecturas del SGP30 con espera no bloqueante
void getAverageSGP30(uint16_t &avgECO2, uint16_t &avgTVOC, int numReadings = 5) {
  uint32_t sumECO2 = 0, sumTVOC = 0;
  int validReadings = 0, readingsDone = 0;
  unsigned long lastMillis = millis();

  while (readingsDone < numReadings) {
    if (millis() - lastMillis >= 500) {
      lastMillis = millis();
      readDHTSensor();  // Refrescar compensación cada ciclo
      if (sgp.IAQmeasure()) {
        if (sgp.eCO2 > 400 && sgp.TVOC > 10) {
          sumECO2  += sgp.eCO2;
          sumTVOC += sgp.TVOC;
          validReadings++;
        }
      }
      readingsDone++;
    }
  }

  avgECO2 = (validReadings > 0) ? sumECO2 / validReadings : sgp.eCO2;
  avgTVOC = (validReadings > 0) ? sumTVOC / validReadings : sgp.TVOC;
}

// Lectura de TDS con espera no bloqueante
void readTDS() {
  terminal.println("Sensor TDS tomando lecturas...");
  int analogSum = 0, readingsDone = 0;
  unsigned long lastMillis = millis();

  while (readingsDone < 20) {
    if (millis() - lastMillis >= 10) {
      lastMillis = millis();
      analogSum += analogRead(TdsSensorPin);
      readingsDone++;
    }
  }

  float avgAnalog = analogSum / 20.0f;
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

// Inicializa y calibra SGP30
void initializeSGP30() {
  if (!sgp.begin()) {
    terminal.println("Sensor SGP30 no detectado :(");
    return;
  }
  terminal.println("Sensor SGP30 precalentando...");

  bool readyToMeasure = false;
  unsigned long start = millis();
  const unsigned long maxWarm = 30000; // 30 segundos

  while (!readyToMeasure && millis() - start < maxWarm) {
    if (sgp.IAQmeasure() && sgp.eCO2 > 400 && sgp.TVOC > 10) {
      readyToMeasure = true;
      eCO2 = sgp.eCO2;
      TVOC = sgp.TVOC;
      break;
    }
    static unsigned long lastDot = 0;
    if (millis() - lastDot >= 1000) {
      lastDot = millis();
      terminal.print('.');
      terminal.flush();  // Fuerza impresión inmediata
      #ifdef BLYNK_CONNECTED
      Blynk.run();      // Mantiene sesión Blynk activa
      #endif
    }
  }

  if (readyToMeasure) {
    terminal.println("");
    terminal.println(" ► SGP30 listo para medir.");
    readDHTSensor();
    getAverageSGP30(eCO2, TVOC);
  } else {
    terminal.println(" ► Timeout. Usando última lectura.");
    sgp.IAQmeasure();
    eCO2 = sgp.eCO2;
    TVOC = sgp.TVOC;
  }

  // Muestra valores y clasifica calidad del aire
  terminal.print("eCO2: "); terminal.print(eCO2);
  terminal.print(" ppm | TVOC: "); terminal.print(TVOC);
  terminal.println(" ppb");
  terminal.print("Calidad del Aire: ");
  if (eCO2 < 600)       terminal.println("EXCELENTE");
  else if (eCO2 < 800)  terminal.println("BUENO");
  else if (eCO2 < 1000) terminal.println("REGULAR");
  else if (eCO2 < 1500) terminal.println("POBRE");
  else                  terminal.println("INACEPTABLE");
}

// Envío de datos a Blynk
void sendDataToBlynk() {
  Blynk.virtualWrite(V0, temperature);  // Temperatura (°C)
  Blynk.virtualWrite(V1, humidity);     // Humedad relativa (%)
  Blynk.virtualWrite(V2, tdsValue);     // TDS (ppm)
  Blynk.virtualWrite(V4, ecValue);      // Conductividad eléctrica (µS/cm)
  Blynk.virtualWrite(V5, TVOC);         // TVOC (ppb)
  Blynk.virtualWrite(V6, eCO2);         // eCO₂ (ppm)
  Blynk.virtualWrite(V7, absH_gm3);     // Humedad absoluta (g/m³)
}

// ---------------- SETUP ----------------

void setup() {
  Serial.begin(9600);
  dht.begin();

  connectToWiFi();       // Conecta WiFi
  initializeSGP30();     // Precalienta y calibra SGP30
  readDHTSensor();       // Primera lectura DHT y compensación
  readTDS();             // Lectura TDS

  sendDataToBlynk();     // Envía datos a la nube

  // Mensaje final y preparación para deepSleep
  terminal.println("Entrando en deepSleep 1 min...");
  terminal.flush();
  #ifdef BLYNK_CONNECTED
  Blynk.run();           // Asegura envío de última trama
  #endif
  delay(500);            // Breve espera para terminal y Blynk

  // IMPORTANTE: para que deepSleep funcione en NodeMCU, une D0 a RST
  ESP.deepSleep(60e6);
}

void loop() {
  // No usado con deepSleep
}
