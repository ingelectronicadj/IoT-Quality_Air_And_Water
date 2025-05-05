// Proyecto de Monitoreo de Calidad del Agua con NodeMCU ESP8266
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

// Pines
#define DHTPIN 2        // D4 en NodeMCU
#define DHTTYPE DHT11
#define TdsSensorPin A0

// Objetos
DHT dht(DHTPIN, DHTTYPE);

// Variables
float temperature = 25.0;
float humidity = 0.0;
float tdsValue = 0.0;

// Constantes
const float VREF = 3.28;         // Voltaje de referencia ADC real medido con multímetro
const int ADC_RES = 1024;        // Resolución ADC (10 bits)
const float TDS_FACTOR = 0.5;    // Factor típico para convertir EC a TDS
const float kValue = 1.0;        // Factor de calibración de la sonda (ajustable)

// WiFi
char ssid[] = "red wifi";
char pass[] = "contrasena";

void setup() {
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();
}

void loop() {
  Blynk.run();

  // Leer temperatura y humedad
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Error al leer del sensor DHT");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.print(" °C\tHumedad: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Leer voltaje del sensor TDS
  int analogValue = analogRead(TdsSensorPin);
  float voltage = analogValue * (VREF / ADC_RES);

  // Convertir a EC en mS/cm según fórmula del fabricante
  float ecValue = (133.42 * voltage * voltage * voltage
                  - 255.86 * voltage * voltage
                  + 857.39 * voltage) * kValue;

  // Compensación por temperatura
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensatedEC = ecValue / compensationCoefficient;

  // Calcular TDS (ppm)
  tdsValue = compensatedEC * TDS_FACTOR; // Convertir EC a TDS
  //tdsValue = tdsValue * 1000; // Convertir a ppm (mg/L)

  Serial.print("Voltaje: ");
  Serial.print(voltage, 2);
  Serial.print(" V\tEC: ");
  Serial.print(compensatedEC, 2);
  Serial.print(" µS/cm\tTDS: ");
  Serial.print(tdsValue, 0);
  Serial.println(" ppm");

  // Clasificación de calidad del agua
  if (tdsValue < 300) {
    Serial.println("Calidad del agua: EXCELENTE");
  } else if (tdsValue < 600) {
    Serial.println("Calidad del agua: BUENA");
  } else if (tdsValue < 900) {
    Serial.println("Calidad del agua: REGULAR");
  } else if (tdsValue < 1200) {
    Serial.println("Calidad del agua: POBRE");
  } else {
    Serial.println("Calidad del agua: INACEPTABLE");
  }

  // Enviar a Blynk
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, tdsValue);

  delay(2000);
}
