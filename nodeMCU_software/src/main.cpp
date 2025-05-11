// Proyecto de Monitoreo de Calidad del Agua y Aire con NodeMCU ESP8266
// Autor: Diego Javier Mena Amado
// Email: ingelectronicadj@gmail.com
// GNU General Public License v3.0

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

// Pines
#define DHTPIN 2        // Pin del sensor DHT11 conectado a D4 en NodeMCU
#define DHTTYPE DHT11   // Tipo de sensor DHT
#define TdsSensorPin A0 // Pin del sensor TDS conectado al pin analógico A0

// Objetos
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SGP30 sgp;
WidgetTerminal terminal(V3); // Terminal de Blynk en el pin V3

// Variables
float temperature = 25.0; // Temperatura inicial
float humidity = 0.0;     // Humedad inicial
float tdsValue = 0.0;     // Valor de TDS
float ecValue = 0.0;      // Conductividad eléctrica
uint16_t TVOC = 0;        // Compuestos orgánicos volátiles totales
uint16_t eCO2 = 0;        // Dióxido de carbono equivalente

// Constantes
const float VREF = 3.28;      // Voltaje de referencia
const int ADC_RES = 1024;     // Resolución del ADC
const float TDS_FACTOR = 0.5; // Factor de conversión TDS
const float kValue = 1.0;     // Constante de calibración EC

// Credenciales de red WiFi
char ssid[] = "WEALTH 2.4G Y 5G";
char pass[] = "Nomelase**";

// Función para calcular la humedad absoluta
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  const float absHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature));
  return static_cast<uint32_t>(1000.0f * absHumidity); // Convertir a mg/m^3
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  int wifiRetries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetries < 20) {
    delay(500);
    wifiRetries++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    terminal.println("Error de conexión WiFi. Reiniciando en 2 minutos...");
    terminal.flush();
    ESP.deepSleep(120e6); // Deep sleep por 2 minutos
  } else {
    terminal.println("Dispositivo conectado a WiFi");
  }

  // Inicializar sensor SGP30
  if (!sgp.begin()) {
    terminal.println("SGP30 no detectado :(");
  } else {
    terminal.println("SGP30 inicializado, en calentamiento...");
    for (int i = 0; i < 60; i++) {
      if (sgp.IAQmeasure()) {
        sgp.setHumidity(getAbsoluteHumidity(25.0, 50.0)); // Humedad absoluta simulada
      }
      delay(500); // Esperar medio segundo entre lecturas
    }
    terminal.println("Calentamiento completado");
  }

  // Leer temperatura y humedad
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    terminal.println("Error al leer del sensor DHT");
  } else {
    terminal.println("TDS inicializado. Promediando 20 lecturas...");
    int analogSum = 0;
    int numReadings = 20;
    for (int i = 0; i < numReadings; i++) {
      analogSum += analogRead(TdsSensorPin);
      delay(10);
    }

    float avgAnalog = analogSum / numReadings;
    float voltage = avgAnalog * (VREF / ADC_RES);
    ecValue = (133.42 * voltage * voltage * voltage - 255.86 * voltage * voltage + 857.39 * voltage) * kValue;
    float compCoeff = 1.0 + 0.02 * (temperature - 25.0);
    float compEC = ecValue / compCoeff;
    tdsValue = compEC * TDS_FACTOR;

    // Leer valores de calidad de aire
    if (sgp.IAQmeasure()) {
      sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));
      TVOC = sgp.TVOC;
      eCO2 = sgp.eCO2;
    } else {
      terminal.println("Error al medir con SGP30");
    }

    // Mostrar datos en terminal
    terminal.print("Temp: "); terminal.print(temperature);
    terminal.print(" °C | Hum: "); terminal.print(humidity); terminal.println(" %");

    terminal.print("EC: "); terminal.print(compEC, 2);
    terminal.print(" µS/cm | TDS: "); terminal.print(tdsValue, 0); terminal.println(" ppm");

    terminal.print("eCO2: "); terminal.print(eCO2); terminal.print(" ppm | TVOC: ");
    terminal.print(TVOC); terminal.println(" ppb");

    // Clasificación de calidad del agua
    if (tdsValue < 300) terminal.println("Agua: EXCELENTE");
    else if (tdsValue < 600) terminal.println("Agua: BUENA");
    else if (tdsValue < 900) terminal.println("Agua: REGULAR");
    else if (tdsValue < 1200) terminal.println("Agua: POBRE");
    else terminal.println("Agua: INACEPTABLE");

    // Enviar datos a Blynk
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, tdsValue);
    Blynk.virtualWrite(V4, compEC);
    Blynk.virtualWrite(V5, TVOC);
    Blynk.virtualWrite(V6, eCO2);
  }

  terminal.println("Entrando en deepSleep 2 min...");
  terminal.flush();
  delay(2000); // Esperar antes de entrar en modo deepSleep
  ESP.deepSleep(120e6); // Dormir por 2 minutos
}

void loop() {
  // No se utiliza con deepSleep
}
