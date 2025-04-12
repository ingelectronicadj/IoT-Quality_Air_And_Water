// Proyecto de Monitoreo de Calidad del Agua con NodeMCU ESP8266
// Autor: Diego Javier Mena Amado
// Email: ingelectronicadj@gmail.com
// GNU General Public License v3.0

// Definición de la plantilla y el token de autenticación de Blynk 
#define BLYNK_TEMPLATE_ID "TMPL2ieIP4YG3"
#define BLYNK_TEMPLATE_NAME "Quality Water"
#define BLYNK_AUTH_TOKEN "EVXWMm138Ebxv07m-b3_FD74GFAN3gP2"
#define BLYNK_PRINT Serial 

#include <Arduino.h>                // Librería base de Arduino para ESP8266 
#include <EEPROM.h>                 // Librería EEPROM para guardar datos en la memoria interna del ESP8266
#include "GravityTDS.h"             // Librería para el sensor de TDS
#include <DHT.h>                    // Librería para el sensor DHT22
#include <ESP8266WiFi.h>            // Librería para la conexión WiFi
#include <BlynkSimpleEsp8266.h>     // Librería Blynk para ESP8266

// Pines
#define DHTPIN 2                    // D4 en NodeMCU
#define DHTTYPE DHT22              // Se cambia el tipo de sensor de DHT11 a DHT22
#define TdsSensorPin A0

// Objetos de sensores
DHT dht(DHTPIN, DHTTYPE);           // Objeto DHT para DHT22
GravityTDS gravityTds;

// Variables
float temperature = 25.0;           // Se mantiene como float para mejor precisión en la compensación
float humidity = 0.0;               // También se cambia a float por precisión
float tdsValue = 0.0;

// Red WiFi
char ssid[] = "DiegoRed";
char pass[] = "nomelase";

void setup()
{
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  dht.begin();                      // Inicialización del sensor DHT22

  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);          // Voltaje de referencia del ADC
  gravityTds.setAdcRange(1024);     // Resolución ADC interna de NodeMCU (10 bits)
  gravityTds.begin();
}

void loop()
{
  Blynk.run();

  // Leer temperatura y humedad directamente como float desde el DHT22
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Verificación de lectura válida
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Error al leer del sensor DHT22");
    return;// Evita continuar si los datos no son validos
  }

  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.print(" °C\tHumedad: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Medición TDS con compensación de temperatura
  gravityTds.setTemperature(temperature); // Mejora significativa en precisión de TDS
  gravityTds.update();
  tdsValue = gravityTds.getTdsValue();

  Serial.print("TDS: ");
  Serial.print(tdsValue, 0);
  Serial.println(" ppm");

  // Clasificación OMS de calidad del agua
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

  // Enviar datos a Blynk
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, tdsValue);

  delay(2000);                      // Intervalo de actualización
}
