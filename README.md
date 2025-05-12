# GorilaTrack 🦍📡

**GorilaTrack** es un sistema de rastreo GPS y comunicación de emergencia diseñado para ofrecer seguridad, confiabilidad y eficiencia energética en situaciones críticas. El dispositivo utiliza un ESP32 conectado a un módulo GPS NEO-6M, un módulo GSM SIM800L, y un sistema de comunicación LoRa SX1276 (En Desarrollo). Integra funcionalidades como envío de SMS con ubicación, llamadas de emergencia, interacción vía Bluetooth y modificación de datos desde una página web accesible sin reprogramar el dispositivo.

<p align="center">
<img width="300" src="extras/img.png" alt="Descripción de la imagen 1" >            <img width="300" src="extras/Logo2.png" alt="Descripción de la imagen 2" >                       
</p>

---

## 🧠 Características principales

- 📍 Obtención de ubicación GPS en [**Tiempo Real**](https://noelcruz1.github.io/GorillaTrack/Tracking-Web/FrontEnd/Html/Track.html) .
- 📲 Envío automático de SMS con enlace de Google Maps.
- 📞 Llamadas de emergencia con hasta 3 intentos si no contestan.
- (En Desarrollo)🔋 Gestión energética optimizada (Deep Sleep, encendido manual).
- 🧠 Configuración remota por Bluetooth mediante página web.
- 💾 Uso de FFat para guardar datos como números telefónicos y mensajes.
- (En Desarrollo)📡 Comunicación LoRa entre múltiples puntos (Puntos Rosa y nodo central).
- 🔧 Totalmente portátil y adaptable a diversas condiciones de operación.

---

## 🔧 Hardware requerido

| Componente            | Modelo/Descripción                       |
|----------------------|-------------------------------------------|
| Microcontrolador     | ESP32-WROOM V3.1 |
| GPS                  | Módulo NEO-6M                            |
| GSM                  | Módulo SIM800L                           |
| Comunicación         | Módulo LoRa SX1276                       |
| Botones físicos      | Para encendido y emergencia              |
| LEDs indicadores     | SMD 0805                      |
| Conversores de voltaje | SM5308, MP1584 |
| Batería              | LiPo 3.7V (dos en paralelo)        |

---

## 📁 Estructura del proyecto

```
GorilaTrack/
│
├── Arduino/
│   ├── main.ino                   # Código principal
│   ├── config.h                   # Pines, configuraciones
│   ├── funciones_sms.h            # Envío de SMS
│   ├── funciones_llamada.h        # Gestión de llamadas
│   ├── funciones_gps.h            # Lectura de GPS
│   ├── funciones_lora.h           # Comunicación LoRa
│   └── funciones_ble.h            # Interfaz BLE y web
│
├── data/
│   └── user_data.txt              # Datos cargados por el usuario
│
├── index.html                 # Página web para editar datos vía BLE
│
└── README.md                      # Este archivo
```

---

## ⚙️ Instalación

1. **Requisitos:**
   - Arduino IDE 2.x
   - Modulos del ESP32 instalados.
   - Librerías necesarias:
     - `TinyGSM`
     - `TinyGPS++`
     - `LoRa`
     - `FS` / `LittleFS`
     - `BLEDevice`, `BLEServer`
     - `Bounce2`
     - `WebServer`

2. **Pasos:**
   - Carga el contenido de `Arduino` al ESP32.
   - Usa el gestor de archivos de Arduino para subir `Innovatek-GT.ino`. Antes de hacerlo Cambia "Partition Scheme" a Ffat.
   - Si deseas editar los datos del usuario, accede al sitio web y activa el Bluetooth del ESP32, enlaza cuando esté activo el modo BLE.

---

## 📱 Configuración vía Bluetooth

- El ESP32 emite una señal BLE detectable por el navegador cuando se presiona el boton trasero 6 veces.
- Desde la web integrada puedes:
  - Cambiar nombre del usuario.
  - Editar número personal y familiar.
  - Modificar el mensaje de emergencia.
  - Activar o desactivar modo de llamada.
- Al guardar los cambios, el ESP32 actualiza su configuración en tiempo real.

---

## 🔐 Seguridad

- Se utilizan botones físicos para evitar activaciones accidentales.
- La Emergencia se activa al presionar el boton EMERGENCIA 2 veces seguidas.
- La Emergencia se desactiva al dejar presionado el boton EMERGENCIA durante 5 segundos.

---

## (En Desarrollo) 🧭  Red de Nodos

Una Red de Nodos esparcidos en lugares seguros alrededor de la ciudad con la funcion de enviar y recibir informacion.
El GorillaTrack incorpara un modulo LoRa SX1278 y se comporta como receptor de señales LoRa desde múltiples Puntos Rosa. Al recibir datos de múltiples nodos, selecciona el nodo más cercano (según coordenadas GPS) y envía su ubicación al número personal del usuario vía SMS, sin intervenir al contacto de emergencia. 

---

## 🚧 Roadmap y mejoras futuras

- 🌐 Versión web más interactiva y con interfaz moderna.
- (En Desarrollo)📡 Soporte para múltiples modos de rastreo (en tiempo real, histórico).
- 🔒 Autenticación básica para la interfaz BLE.
- 🛰️ Transmisión por satélite en zonas sin cobertura GSM.
- (En Desarrollo)🔋 Optimización extrema para autonomía de meses.

---

## 🧪 Créditos

Este proyecto fue desarrollado pensando en la seguridad personal y la tecnología accesible.  
Gracias por usar GorilaTrack.
