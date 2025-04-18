/* Innovatec ----- TecNM ----- GorilaTrack
 *  Codigo desarrollado por Noel Cruz.
 *  
 *  Desarrollo del codigo IA.
 *  ChatGPT: https://chatgpt.com/share/67f6dc4c-15b4-800f-b5d1-09d732d7e468
 *  DeepSeek: https://chat.jaay.fun/9sgpe7i
 *  
 *  Referencias:
 *  https://justdoelectronics.com/build-your-smart-gps-tracker-system-using-arduino
 *  https://lastminuteengineers.com/neo6m-gps-arduino-tutorial/
 *  https://drive.google.com/file/d/1BzM5DfcEaQ4A3Eo1Z3GSNZLdWvmttOiO/view
 *  https://github.com/techiesms/GPS-Tracker-via-GSM-using-TTGO-TCALL-Module-
  */
// Definir el modelo GSM ANTES de incluir TinyGsmClient
#define TINY_GSM_MODEM_SIM800

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <TinyGsmClient.h>
#include <esp_sleep.h>
#include <Bounce2.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <FS.h>
#include <FFat.h>


// Pines GPS
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

// Pines GSM
#define MODEM_RX 26
#define MODEM_TX 27
#define GSM_BAUD 9600

// Pines de botones
#define BTN_POWER 4
#define BTN_EMERGENCY 5

// BLE
#define SERVICE_UUID            "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_NOMBRE_UUID        "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_PERSONAL_UUID      "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_FAMILIAR_UUID      "6e400004-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_MENSAJE_UUID       "6e400005-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_MODO_LLAMADA_UUID  "6e400006-b5a3-f393-e0a9-e50e24dcca9e"

BLECharacteristic* charNombre;
BLECharacteristic* charPersonal;
BLECharacteristic* charFamiliar;
BLECharacteristic* charMensaje;
BLECharacteristic* charModo;

// Pines LED
#define LED_POWER 12 // Verde
#define LED_DATA 13 // Azul
#define LED_GSM 14 // Naranja

/*
╔══════════════╦════════════════════════════════════════════════════════════╗
║    LED       ║                       ESTADO / FUNCIÓN                    ║
╠══════════════╬════════════════════════════════════════════════════════════╣
║ 🟢 LED_POWER  ║ • Encendido fijo mientras el sistema está activo          ║
║  (pin 12)    ║ • Parpadea 5 veces antes de entrar en deep sleep          ║
║  (Verde)     ║ • Se apaga al entrar en modo de bajo consumo             ║
╠══════════════╬════════════════════════════════════════════════════════════╣
║ 🔴 LED_DATA   ║ • Se enciende al enviar un SMS                            ║
║  (pin 13)    ║ • Parpadea 3 veces si el SMS fue enviado correctamente    ║
║   (Azul)     ║ • Parpadea 6 veces (rápido) si falla el envío del SMS     ║
║              ║ • Parpadea constantemente cuando emergencia está activa   ║
║              ║ • Ya NO indica nada relacionado con llamadas              ║
╠══════════════╬════════════════════════════════════════════════════════════╣
║ 🔵 LED_GSM   ║ • Parpadea durante una llamada de emergencia activa       ║
║  (pin 14)    ║ • Se apaga al finalizar o cancelar la llamada             ║
║  (Naranja)   ║ • Ya NO se enciende durante la inicialización del módem  ║
╚══════════════╩════════════════════════════════════════════════════════════╝
*/
// Datos Personales
String numeroPersonal = "";
String numeroFamiliar = "";
String nombreUsuario = "";
String mensajeEmergencia = "";
bool modoLlamada = false;  // "on" o "off"
 
// Configuración
const unsigned long POWER_OFF_HOLD_TIME = 3000;
const unsigned long EMERGENCY_HOLD_TIME = 2000;
const unsigned long EMERGENCY_CANCEL_HOLD_TIME = 5000;
const unsigned long SMS_REPEAT_INTERVAL = 60000;  // 60 segundos
const unsigned long GSM_INIT_TIMEOUT = 10000;
const unsigned long LED_BLINK_INTERVAL = 200;
//BLE + LittleFS (Editar variables)
const int NUM_CLICKS_BLE = 6;
const unsigned long CLICK_TIMEOUT = 3000;

// Instancias
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);
HardwareSerial sim800(1);
TinyGsm modem(sim800);
Bounce powerBtn = Bounce();
Bounce emergencyBtn = Bounce();

// Variables de estado
bool gsmReady = false;
unsigned long powerBtnPressedTime = 0;
unsigned long emergencyBtnPressedTime = 0;
bool powerBtnActive = false;
bool emergencyBtnActive = false;
bool ledDataState = false;
unsigned long lastLedBlink = 0;
bool emergenciaActiva = false;
unsigned long lastSMSTime = 0;
bool llamadaRealizada = false;

unsigned int clickCount = 0;
unsigned long lastClickTime = 0;
bool modoConfiguracionBLE = false;


// BLE ---------------------------------------------------------------------------------------------------------BLE
class ConfigCallback : public BLECharacteristicCallbacks {
  String fileName;
  String* targetVar;

public:
  ConfigCallback(String fname, String* varRef) : fileName(fname), targetVar(varRef) {}

  void onWrite(BLECharacteristic* pCharacteristic) override {
    // Leer el valor escrito
    String value = pCharacteristic->getValue().c_str();
    if (value.length() > 0) {
      // Guardar en la variable apuntada
      *targetVar = value;

      // Escribir en FFat
      File f = FFat.open(("/" + fileName + ".txt").c_str(), "w");
      if (f) {
        f.print(*targetVar);
        f.close();
        Serial.println("Archivo actualizado: " + fileName + ".txt = " + *targetVar);
      } else {
        Serial.println("❌ Error al escribir " + fileName + ".txt");
      }

      // Si es modo_llamada, ajustar el bool, dar feedback y apagar BLE
      if (fileName == "modo_llamada") {
        // Normalizar el texto a "on" u "off"
        *targetVar = (*targetVar == "on") ? "on" : "off";
        // Actualizar la variable global
        modoLlamada = (*targetVar == "on");

        // Parpadeo del LED_DATA como confirmación
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_DATA, HIGH);
          delay(150);
          digitalWrite(LED_DATA, LOW);
          delay(150);
        }

        // Apagar el BLE
        Serial.println("🛑 Configuración finalizada. Apagando BLE.");
        BLEDevice::deinit(true);
        modoConfiguracionBLE = false;
      }
    }
  }
};

// BLUETOOTH----------------------------------------------------------------------------------------------------BLUETOOTH

void setup() {
  pinMode(BTN_POWER, INPUT_PULLUP);
  pinMode(BTN_EMERGENCY, INPUT_PULLUP);
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_DATA, OUTPUT);
  pinMode(LED_GSM, OUTPUT);

  powerBtn.attach(BTN_POWER);
  powerBtn.interval(25);
  emergencyBtn.attach(BTN_EMERGENCY);
  emergencyBtn.interval(25);

  digitalWrite(LED_POWER, HIGH);

  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);

  

  if (!FFat.begin()) {
  Serial.println("Error al montar FFat");
} else {
  cargarDatosDesdeFS();
}


  initGSMModem();

  esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_POWER, LOW);
}

void loop() {
  powerBtn.update();
  emergencyBtn.update();

  handlePowerButton();
  handleEmergencyButton();

  updateLedFeedback();

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Solo se actualiza el GPS cuando es necesario, por ejemplo, al enviar SMS en emergencia
  if (emergenciaActiva && millis() - lastSMSTime >= SMS_REPEAT_INTERVAL) {
    enviarSMS();
    lastSMSTime = millis();
  }
   detectarClicksPower();
}

 void detectarClicksPower() {
  powerBtn.update();
  if (powerBtn.fell()) {
    unsigned long now = millis();
    // Si ha pasado demasiado tiempo reinicia conteo
    if (now - lastClickTime > CLICK_TIMEOUT) {
      clickCount = 0;
    }
    lastClickTime = now;
    clickCount++;

    // Al llegar a 6, activa BLE
    if (clickCount >= NUM_CLICKS_BLE && !modoConfiguracionBLE) {
      clickCount = 0;
      modoConfiguracionBLE = true;
      Serial.println("🔧 6 clics detectados: activando BLE para configuración");
      iniciarBLE();
    }
  }
}


void initGSMModem() {
  Serial.println("Iniciando módem GSM...");

  sim800.begin(GSM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(1000);

  unsigned long startTime = millis();
  gsmReady = false;

  while (millis() - startTime < GSM_INIT_TIMEOUT && !gsmReady) {
    Serial.print(".");
    if (modem.restart()) {
      if (modem.init()) {
        if (modem.waitForNetwork(30000)) {
          if (modem.isNetworkConnected()) {
            gsmReady = true;
            Serial.println("\nMódem GSM listo");

            modem.sendAT("+CMGF=1");
            if (modem.waitResponse(1000L) == 1) {
              Serial.println("Modo SMS configurado");
            } else {
              Serial.println("Error configurando modo SMS");
            }

            Serial.print("Intensidad de señal: ");
            Serial.println(modem.getSignalQuality());
          }
        }
      }
    }
    delay(500);
  }

  if (!gsmReady) {
    Serial.println("\nFallo al iniciar módem GSM");
  }
}

  

void cargarDatosDesdeFS() {
  struct ConfigEntry {
    const char* fileName;
    String* variable;
  };

  ConfigEntry config[] = {
    { "numero_personal.txt", &numeroPersonal },
    { "numero_familiar.txt", &numeroFamiliar },
    { "nombre_Usuario.txt", &nombreUsuario },
    { "mensaje_emergencia.txt", &mensajeEmergencia },
    { "modo_llamada.txt", &mensajeEmergencia }  // temporal, lo manejaremos diferente abajo
  };

  for (auto& entry : config) {
    File f = FFat.open(("/" + String(entry.fileName)).c_str(), "r");
    if (f) {
      *entry.variable = f.readStringUntil('\n');
      entry.variable->trim();
      f.close();
      Serial.printf("%s cargado: %s\n", entry.fileName, entry.variable->c_str());
    } else {
      Serial.printf("Archivo %s no encontrado\n", entry.fileName);
    }
  }

  // Carga especial para modo_llamada como bool
  File modoFile = FFat.open("/modo_llamada.txt", "r");
  if (modoFile) {
    String valor = modoFile.readStringUntil('\n');
    valor.trim();
    modoLlamada = (valor == "on");
    modoFile.close();
    Serial.printf("modo_llamada: %s\n", modoLlamada ? "on" : "off");
  }
}


// BLE
void iniciarBLE() {
  BLEDevice::init("GorilaTrack");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

BLECharacteristic *charNombre = pService->createCharacteristic(CHAR_NOMBRE_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
BLECharacteristic *charPersonal = pService->createCharacteristic(CHAR_PERSONAL_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
BLECharacteristic *charFamiliar = pService->createCharacteristic(CHAR_FAMILIAR_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
BLECharacteristic *charMensaje = pService->createCharacteristic(CHAR_MENSAJE_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
BLECharacteristic *charModo = pService->createCharacteristic(CHAR_MODO_LLAMADA_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);


  charNombre->setCallbacks(new ConfigCallback("nombre_Usuario", &nombreUsuario));
  charPersonal->setCallbacks(new ConfigCallback("numero_personal", &numeroPersonal));
  charFamiliar->setCallbacks(new ConfigCallback("numero_familiar", &numeroFamiliar));
  charMensaje->setCallbacks(new ConfigCallback("mensaje_emergencia", &mensajeEmergencia));
  charModo->setCallbacks(new ConfigCallback("modo_llamada", &mensajeEmergencia));  // <- Se usa para bool

charNombre->setValue(nombreUsuario.c_str());
  charPersonal->setValue(numeroPersonal.c_str());
  charFamiliar->setValue(numeroFamiliar.c_str());
  charMensaje->setValue(mensajeEmergencia.c_str());
charModo->setValue(modoLlamada ? "on" : "off");


  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("🔵 BLE activo: GorilaTrack visible por Bluetooth");
}


void handlePowerButton() {
  powerBtn.update();  // Asegúrate de llamarlo también aquí por si se omite en loop

  if (powerBtn.fell()) {
    powerBtnPressedTime = millis();
    powerBtnActive = true;
    Serial.println("Botón de poder PRESIONADO");
  }

  if (powerBtn.rose()) {
    powerBtnActive = false;
    Serial.println("Botón de poder LIBERADO antes del tiempo requerido");
  }

  if (powerBtnActive) {
    unsigned long heldTime = millis() - powerBtnPressedTime;
    Serial.print("Tiempo presionado: ");
    Serial.print(heldTime);
    Serial.println(" ms");

    if (heldTime >= POWER_OFF_HOLD_TIME) {
      Serial.println("Tiempo suficiente detectado. Apagando...");
      apagarDispositivo();
    }
  }
}

void handleEmergencyButton() {
  if (emergencyBtn.fell()) {
    emergencyBtnPressedTime = millis();
    emergencyBtnActive = true;
    Serial.println("Botón emergencia presionado");
  }

  if (emergencyBtn.rose()) {
    emergencyBtnActive = false;
    digitalWrite(LED_DATA, LOW);
    Serial.println("Botón emergencia liberado");
  }

  if (emergencyBtnActive) {
    unsigned long holdTime = millis() - emergencyBtnPressedTime;

    // Cancelar emergencia si se mantiene presionado 5 segundos
    if (emergenciaActiva && holdTime >= EMERGENCY_CANCEL_HOLD_TIME) {
      Serial.println("Emergencia cancelada por usuario");
      modem.callHangup();
      emergenciaActiva = false;
      llamadaRealizada = false;
      digitalWrite(LED_DATA, LOW);
      return;
    }

    // Activar emergencia
    if (!emergenciaActiva && holdTime >= EMERGENCY_HOLD_TIME) {
      if (gsmReady) {
        Serial.println("Activando protocolo de emergencia");
        emergenciaActiva = true;
        lastSMSTime = millis();
        llamadaRealizada = false;
        enviarSMS();
        hacerLlamada();
      } else {
        Serial.println("Error: Módem GSM no disponible");
        for (int i = 0; i < 10; i++) {
          digitalWrite(LED_DATA, HIGH);
          delay(100);
          digitalWrite(LED_DATA, LOW);
          delay(100);
        }
      }
      emergencyBtnActive = false;
    }
  }
}

void updateLedFeedback() {
  if (emergencyBtnActive || emergenciaActiva) {
    if (millis() - lastLedBlink >= LED_BLINK_INTERVAL) {
      lastLedBlink = millis();
      ledDataState = !ledDataState;
      digitalWrite(LED_DATA, ledDataState);
    }
  }
}

// Cambios aplicados en función del código base
// LED_DATA = Estados SMS
// LED_GSM  = Estados llamada

void enviarSMS() {
  if (!gps.location.isValid()) {
    Serial.println("Error: No hay posición GPS válida");
    return;
  }

  if (!gsmReady) {
    Serial.println("Error: Módem no está listo");
    return;
  }

  String sms = mensajeEmergencia + "\n";
  sms += nombreUsuario + " se encuentra aquí:\n";
  sms += "http://maps.google.com/maps?q=loc:";
  sms += String(gps.location.lat(), 6);
  sms += ",";
  sms += String(gps.location.lng(), 6);
  sms += "\nPrecisión: ";
  sms += gps.hdop.value();
  sms += "m | Satélites: ";
  sms += gps.satellites.value();

  digitalWrite(LED_DATA, HIGH);

  for (int intento = 1; intento <= 3; intento++) {
    Serial.print("Intentando enviar SMS (intento ");
    Serial.print(intento);
    Serial.println(")");

    if (modem.sendSMS(numeroFamiliar.c_str(), sms.c_str())) {
      Serial.println("SMS enviado correctamente");
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_DATA, HIGH); delay(150);
        digitalWrite(LED_DATA, LOW); delay(150);
      }
      return;
    }
    delay(1000);
  }

  Serial.println("Error: No se pudo enviar SMS después de 3 intentos");
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_DATA, HIGH); delay(100);
    digitalWrite(LED_DATA, LOW); delay(100);
  }
}


void hacerLlamada() {
if (llamadaRealizada || !gsmReady || !modoLlamada) return;

  Serial.println("Iniciando protocolo de llamadas de emergencia...");
  llamadaRealizada = true;

  for (int intento = 1; intento <= 3; intento++) {
    Serial.print("Intentando llamada (intento ");
    Serial.print(intento);
    Serial.println(")");

if (modem.callNumber(numeroFamiliar.c_str())) {

      Serial.println("Llamando...");

      unsigned long callStart = millis();
      bool llamadaCancelada = false;

      while (millis() - callStart < 15000) {  // 15 segundos
        // Parpadeo LED_GSM durante llamada
        digitalWrite(LED_GSM, millis() % 1000 < 500);
        delay(100);

        emergencyBtn.update();
        if (emergencyBtn.read() == LOW && millis() - emergencyBtnPressedTime >= EMERGENCY_CANCEL_HOLD_TIME) {
          Serial.println("Llamada cancelada por usuario");
          modem.callHangup();
          digitalWrite(LED_GSM, LOW);
          emergenciaActiva = false;
          llamadaRealizada = false;
          llamadaCancelada = true;
          break;
        }
      }

      modem.callHangup();
      digitalWrite(LED_GSM, LOW);

      if (llamadaCancelada) return;

      Serial.println("Llamada finalizada (sin respuesta)");
    } else {
      Serial.println("Error al iniciar llamada");
    }

    delay(2000);  // Espera entre intentos
  }

  Serial.println("No se logró completar la llamada tras 3 intentos");
}
void apagarDispositivo() {
  Serial.println("Apagando dispositivo...");

  modem.poweroff();

  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_POWER, !digitalRead(LED_POWER));
    digitalWrite(LED_DATA, !digitalRead(LED_DATA));
    delay(100);
  }

  digitalWrite(LED_POWER, LOW);
  digitalWrite(LED_DATA, LOW);
  digitalWrite(LED_GSM, LOW);

  pinMode(LED_POWER, INPUT);
  pinMode(LED_DATA, INPUT);
  pinMode(LED_GSM, INPUT);

  esp_deep_sleep_start();
}
