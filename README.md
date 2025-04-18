# GorillaTrack
Sistema Portátil de Rastreo y Emergencia para Zonas Vulnerables
Autor: Noel Cruz | Institución: Innovatec - TecNM

Descripción

GorilaTrack es un dispositivo autónomo de seguridad personal, diseñado para enviar alertas de emergencia con geolocalización precisa y realizar llamadas automáticas a un contacto designado. Funciona de forma independiente de smartphones o Internet, y está optimizado para bajo consumo energético. En fases futuras, integrará comunicación LoRa para crear una red comunitaria de "Puntos Rosa".

Características

📲 Envío de SMS de emergencia con enlace a Google Maps

📞 Hasta 3 intentos de llamada automática

🛰️ Geolocalización precisa con GPS NEO-6M

🔋 Modo deep sleep (consumo < 10 μA) y botón de encendido/apagado

🔴 LEDs indicadores:

LED_POWER (Verde): Estado de energía

LED_DATA (Azul): Estado de SMS

LED_GSM (Naranja): Estado de llamada

🚫 Protocolo anti-falsas alarmas (confirmación por presión prolongada)

🔄 Reenvío de SMS cada 60 segundos mientras la emergencia está activa

🔭 Plan de integración futura con LoRa (SX1276) y red de Puntos Rosa

Contenido del Repositorio

src/: Código fuente Arduino/ESP32

docs/: Documentación técnica y esquemas de conexión

README.md: Archivo principal de presentación y guía de uso

LICENSE: Licencia de código abierto (MIT)

Requisitos de Hardware

Componente

Detalle

ESP32 (C3, S3 u otro)

Microcontrolador principal

SIM800L

Módulo GSM para SMS y llamadas

GPS NEO-6M

Módulo GPS NMEA

Fuente de alimentación

LiPo 3.7V (con TP4056) o 5V estabilizado

Convertidor DC-DC

Buck a 4.0V (ej. MP1584, LM2596)

LEDs SMD 1206

3 colores (12, 13, 14)

Botones

BTN_POWER (GPIO4), BTN_EMERGENCY (GPIO5)

(Opcional) LoRa SX1276

Para futuras fases

Conexiones (Pinout)

ESP32            SIM800L/GPS/LEDs
------           ----------------
GPIO4  - BTN_POWER
GPIO5  - BTN_EMERGENCY
GPIO12 - LED_POWER (Verde)
GPIO13 - LED_DATA  (Azul)
GPIO14 - LED_GSM   (Naranja)
GPIO16 - GPS RXD2
GPIO17 - GPS TXD2
GPIO26 - SIM800L RX
GPIO27 - SIM800L TX
Vin    - 4.0V (Buck Converter)
GND    - Tierra común

Instalación de Software

Clona este repositorio:



git clone https://github.com/tu_usuario/GorilaTrack.git cd GorilaTrack/src

2. Abre el proyecto en **Arduino IDE** o **PlatformIO**.
3. Instala las siguientes librerías desde el Library Manager:
   - TinyGPSPlus
   - TinyGsmClient
   - Bounce2
4. Selecciona la placa **ESP32** y el puerto correspondiente.
5. Compila y sube el código.

---

## Uso

1. Enciende el dispositivo (LED_POWER en verde).
2. Presiona **BTN_EMERGENCY** por >2 segundos para activar la emergencia.
3. El dispositivo enviará un SMS con tu ubicación y luego intentará llamar.
4. Mantén presionado **BTN_EMERGENCY** por >5 segundos para cancelar.
5. El dispositivo entrará en deep sleep al apagar con **BTN_POWER** (>3 segundos).

---

## Indicadores LED

| LED         | Color   | Función Principal                                        |
|-------------|---------|----------------------------------------------------------|
| LED_POWER   | 🟢 Verde| Energía: encendido / parpadeo antes de deep sleep        |
| LED_DATA    | 🔵 Azul | SMS: envío (fijo), éxito (3 parpadeos), fallo (6 rápidos)|
| LED_GSM     | 🟠 Naranja| Llamada: parpadeo durante llamada, apaga al finalizar    |

---

## Roadmap

| Fase   | Estado          | Descripción                                    |
|--------|-----------------|------------------------------------------------|
| Fase 1 | ✅ Completado   | Protótipo con SMS/GPS/Llamada                  |
| Fase 2 | 🚧 En desarrollo | Integración LoRa y red de Puntos Rosa          |
| Fase 3 | 📝 Planificado  | Plataforma web/app de monitoreo                |
| Fase 4 | 📝 Planificado  | Sensores adicionales y análisis predictivo     |

---

## Contribuciones

¡Las contribuciones son bienvenidas! Por favor, abre una issue o un pull request con mejoras, correcciones o nuevas funcionalidades.

---

## Licencia

Este proyecto está licenciado bajo **MIT License**. Consulta el archivo `LICENSE` para más detalles.

---

*Desarrollado por Noel Cruz y colaboradores.*

