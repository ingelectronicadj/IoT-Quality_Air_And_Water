# Proyecto de Monitoreo de Calidad del Agua con NodeMCU ESP8266

## Descripción
Este proyecto consiste en un sistema de monitoreo básico de calidad del agua utilizando una placa NodeMCU ESP8266. El sistema emplea sensores para medir:

- **TDS (Total de Sólidos Disueltos)** en el agua.
- **Temperatura y humedad ambiental** mediante un sensor **DHT11**.

Los datos recopilados se envían a la plataforma en la nube **[Blynk](https://blynk.io/)** para su visualización en tiempo real desde dispositivos móviles o paneles web.

---

## Componentes Utilizados

| Componente          | Descripción                               |
|---------------------|-------------------------------------------|
| NodeMCU ESP8266     | Microcontrolador con WiFi integrado       |
| Sensor TDS (DFRobot)| Medidor de sólidos disueltos en agua      |
| Sensor DHT11        | Sensor de temperatura y humedad           |
| Resistencia 10kΩ    | Pull-up para el pin de datos del DHT11    |
| Protoboard y cables | Para conexiones físicas                   |

---

## Esquema de Conexión

_A continuación se muestra el diagrama de conexiones físicas del proyecto:_

![Esquema de conexión](./schema/prototype_bb.png)

---

## Librerías Arduino Requeridas

Asegúrate de instalar las siguientes librerías en el IDE de Arduino:

- `GravityTDS-master`
- `DHT11`
---

## Funcionamiento

1. El sensor TDS mide la cantidad de sólidos disueltos en el agua y envía una señal analógica.
2. El sensor DHT11 mide temperatura y humedad ambiental.
3. El NodeMCU ESP8266 recoge los datos y los envía a la aplicación móvil mediante Blynk.
4. En la app Blynk puedes visualizar en tiempo real las condiciones del agua y el entorno.

---

## Plataforma en la Nube: Blynk

Usamos **[Blynk](https://blynk.io/)** como plataforma IoT para visualizar los datos de los sensores en tiempo real. Blynk permite crear interfaces móviles atractivas con botones, gráficas y widgets que interactúan directamente con tu hardware.

---

## Posibles mejoras futuras

- Cambiar el sensor DHT11 por un DHT22 para mayor precisión.
- Añadir más sensores (pH, turbidez, etc.).
- Implementar alertas automáticas por condiciones críticas.
- Usar un panel solar y batería recargable para operación autónoma.

---

## Licencia

Este proyecto es de código abierto y puede ser utilizado con fines educativos, personales o de investigación.
