#include <Arduino.h>
#include <EEPROM.h>
#include "GravityTDS.h"
#include <DHT11.h>

#define TdsSensorPin A0  // Entrada analógica
#define DHT11_PIN 2      // GPIO2 / D4

GravityTDS gravityTds;
DHT11 dht11(DHT11_PIN);

float temperature = 25.0;
float tdsValue = 0;

void mostrarCalidadAgua(float tds) {
    if (tds < 300) {
        Serial.println("🔵 Calidad del agua: EXCELENTE (<300 ppm)");
    } else if (tds >= 300 && tds < 600) {
        Serial.println("🟢 Calidad del agua: BUENA (300-600 ppm)");
    } else if (tds >= 600 && tds < 900) {
        Serial.println("🟡 Calidad del agua: REGULAR (600-900 ppm)");
    } else if (tds >= 900 && tds <= 1200) {
        Serial.println("🟠 Calidad del agua: POBRE (900-1200 ppm)");
    } else {
        Serial.println("🔴 Calidad del agua: INACEPTABLE (>1200 ppm)");
    }
}

void setup() {
    Serial.begin(9600);

    gravityTds.setPin(TdsSensorPin);
    gravityTds.setAref(3.3);        // Voltaje de referencia en ESP8266
    gravityTds.setAdcRange(4096);   // Resolución ADC 12 bits (si se usa ADC externo)
    gravityTds.begin();
}

void loop() {
    int temp = 0;
    int humidity = 0;

    int result = dht11.readTemperatureHumidity(temp, humidity);

    if (result == 0) {
        temperature = temp;

        gravityTds.setTemperature(temperature);
        gravityTds.update();
        tdsValue = gravityTds.getTdsValue();

        Serial.print("🌡️ Temperatura: ");
        Serial.print(temperature);
        Serial.print(" °C\t");

        Serial.print("💧 Humedad: ");
        Serial.print(humidity);
        Serial.print(" %\t");

        Serial.print("🔬 TDS: ");
        Serial.print(tdsValue, 0);
        Serial.println(" ppm");

        mostrarCalidadAgua(tdsValue);
    } else {
        Serial.print("⚠️ Error DHT11: ");
        Serial.println(DHT11::getErrorString(result));
    }

    Serial.println("-----------------------------");
    delay(2000);
}