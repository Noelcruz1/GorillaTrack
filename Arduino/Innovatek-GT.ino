/* GorilaTrack con FreeRTOS - Integración completa
 *  Incluye: GSM, GPS, Botón de emergencia, FFat, BLE, LEDs, FreeRTOS
 */

 /*Configuraciones de la SIM*/
#define TINY_GSM_MODEM_SIM800

// Your GPRS credentials, if any
const char apn[] = "internet.itelcel.com";
// const char apn[] = "ibasis.iot";
const char gprsUser[] = "webgprs";
const char gprsPass[] = "webgprs2002";


#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <TinyGsmClient.h>
#include <HardwareSerial.h>
#include <Bounce2.h>
#include <FS.h>
#include <FFat.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>



// =================== Pines ===================
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600
#define MODEM_RX 3
#define MODEM_TX 1
#define GSM_BAUD 38400
#define BTN_EMERGENCY 5
#define BTN_BLE 4
#define BLE_CLICK_COUNT 6
#define BLE_CLICK_TIMEOUT 3000
#define LED_POWER 12
#define LED_DATA 13
#define LED_GSM 14

// ================ BLE UUIDs =================
#define SERVICE_UUID            "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_NOMBRE_UUID        "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_PERSONAL_UUID      "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_FAMILIAR_UUID      "6e400004-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_MENSAJE_UUID       "6e400005-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_MODO_LLAMADA_UUID  "6e400006-b5a3-f393-e0a9-e50e24dcca9e"

// ============== Instancias ===================
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);
HardwareSerial sim800(1);
TinyGsm modem(sim800);
Bounce emergencyBtn = Bounce();
QueueHandle_t xQueueEventos;

// ================= Variables =================
String numeroPersonal = "";
String numeroFamiliar = "";
String nombreUsuario = "";
String mensajeEmergencia = "";

SemaphoreHandle_t xMutexGSM;

bool modoLlamada = false;
bool emergenciaActiva = false;
bool llamadaRealizada = false;

bool smsEnviadoOK = false;
bool smsEnviadoFallo = false;
bool errorGSM = false;  // Variable para controlar el estado GSM
bool bleActivo = false; // Variable para controlar el estado BLE
bool parpadeoEmergenciaActivo = false;
bool llamadaActiva = false;


unsigned long lastSMSTime = 0;
const unsigned long SMS_INTERVAL = 60000;

unsigned long bleInactiveTimeout = 300000; // 5 minutos
unsigned long bleActivatedTime = 0;

// =================== BLE =====================
BLECharacteristic* charNombre;
BLECharacteristic* charPersonal;
BLECharacteristic* charFamiliar;
BLECharacteristic* charMensaje;
BLECharacteristic* charModo;
bool modoConfiguracionBLE = false;

// ============= BLE Callbacks ================
class ConfigCallback : public BLECharacteristicCallbacks {
  String fileName;
  String* targetVar;

public:
  ConfigCallback(String fname, String* varRef) : fileName(fname), targetVar(varRef) {}
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String value = pCharacteristic->getValue().c_str();
    if (value.length() > 0) {
      *targetVar = value;
      File f = FFat.open(("/" + fileName + ".txt").c_str(), "w");
      if (f) {
        f.print(*targetVar);
        f.close();
        Serial.println("Actualizado: " + fileName);
      }
      if (fileName == "modo_llamada") modoLlamada = (*targetVar == "on");
    }
  }
};

void iniciarBLE() {
  BLEDevice::init("GorilaTrack");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  charNombre = pService->createCharacteristic(CHAR_NOMBRE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  charPersonal = pService->createCharacteristic(CHAR_PERSONAL_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  charFamiliar = pService->createCharacteristic(CHAR_FAMILIAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  charMensaje = pService->createCharacteristic(CHAR_MENSAJE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  charModo = pService->createCharacteristic(CHAR_MODO_LLAMADA_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  charNombre->setCallbacks(new ConfigCallback("nombre_Usuario", &nombreUsuario));
  charPersonal->setCallbacks(new ConfigCallback("numero_personal", &numeroPersonal));
  charFamiliar->setCallbacks(new ConfigCallback("numero_familiar", &numeroFamiliar));
  charMensaje->setCallbacks(new ConfigCallback("mensaje_emergencia", &mensajeEmergencia));
  charModo->setCallbacks(new ConfigCallback("modo_llamada", &mensajeEmergencia));

  charNombre->setValue(nombreUsuario.c_str());
  charPersonal->setValue(numeroPersonal.c_str());
  charFamiliar->setValue(numeroFamiliar.c_str());
  charMensaje->setValue(mensajeEmergencia.c_str());
  charModo->setValue(modoLlamada ? "on" : "off");

  pService->start();
  pServer->getAdvertising()->start();
  bleActivo = true; // Activar el indicador BLE
  bleActivatedTime = millis();
modoConfiguracionBLE = true;
  Serial.println("BLE iniciado");
  vTaskDelete(NULL);  // Cierra la tarea al terminar
}

void cargarDatosDesdeFS() {
  struct ConfigEntry { const char* nombre; String* ref; } configs[] = {
    {"numero_personal.txt", &numeroPersonal},
    {"numero_familiar.txt", &numeroFamiliar},
    {"nombre_Usuario.txt", &nombreUsuario},
    {"mensaje_emergencia.txt", &mensajeEmergencia},
  };

  for (auto& entry : configs) {
    File f = FFat.open(("/" + String(entry.nombre)).c_str(), "r");
    if (f) *entry.ref = f.readStringUntil('\n');
    f.close();
  }
  File modoFile = FFat.open("/modo_llamada.txt", "r");
  if (modoFile) modoLlamada = (modoFile.readStringUntil('\n') == "on");
  modoFile.close();
}

void enviarSMS() {
  if (!gps.location.isValid()) return;

  if (xSemaphoreTake(xMutexGSM, pdMS_TO_TICKS(5000))) { // Esperar hasta 5 segundos
  String sms = "GorillaTrack \n";
    sms += mensajeEmergencia + "\n" + nombreUsuario + ": http://maps.google.com/maps?q=loc:";
    sms += String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
    sms += "\nPrecision: " + String(gps.hdop.value()) + "m";

    bool exito = false;

    for (int i = 1; i <= 3; i++) {
      Serial.printf("Intento de SMS (%d)\n", i);
      if (modem.sendSMS(numeroFamiliar.c_str(), sms.c_str())) {
        Serial.println("✅ SMS enviado correctamente.");
        exito = true;
        break;
      }
      delay(1000);
    }

    smsEnviadoOK = exito;
    smsEnviadoFallo = !exito;

    xSemaphoreGive(xMutexGSM); // Liberar
  } else {
    Serial.println("❌ No se pudo obtener acceso al GSM para enviar SMS.");
    smsEnviadoFallo = true;
  }
}




void hacerLlamada() {
  if (!modoLlamada || llamadaRealizada) return;

  if (xSemaphoreTake(xMutexGSM, pdMS_TO_TICKS(5000))) { // Esperar hasta 5 segundos
    llamadaRealizada = true;
    Serial.println("Iniciando llamada...");

    bool llamadaExitosa = false;

    for (int intento = 1; intento <= 3; intento++) {
      Serial.printf("📞 Intento de llamada %d...\n", intento);

      if (modem.callNumber(numeroFamiliar.c_str())) {
        llamadaActiva = true;
        digitalWrite(LED_GSM, HIGH); // LED fijo durante intento

        unsigned long inicio = millis();
        while (millis() - inicio < 15000) { // Esperar 15s en la llamada
          vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        llamadaActiva = false;       //  Apaga lógica de llamada
parpadeoEmergenciaActivo = false; //  (por si parpadeaba)
digitalWrite(LED_GSM, LOW);   //  Apaga LED físicamente


        Serial.println("❌ Llamada no contestada o colgada después de 15s.");

        delay(2000); // Esperar antes del siguiente intento
      } else {
        Serial.println("⚠️ No se pudo iniciar la llamada.");
        delay(2000); // Esperar antes de intentar otra vez
      }
    }

    // Después de 3 intentos
    llamadaActiva = false;
    digitalWrite(LED_GSM, LOW); // 🔥 Apagamos el LED por seguridad
    Serial.println("❌ Llamadas fallidas después de 3 intentos.");

    xSemaphoreGive(xMutexGSM); // Liberar
  } else {
    Serial.println("❌ No se pudo obtener acceso al GSM para hacer llamada.");
  }
}





void taskBoton(void *param) {
  unsigned long holdStart = 0;
  unsigned long lastClickTime = 0;
  int clickCount = 0;
  const unsigned long clickTimeout = 500; // 500ms entre clics
  bool buttonHeld = false;

  for (;;) {
    emergencyBtn.update();

    if (emergencyBtn.fell()) {
      unsigned long now = millis();
      if (now - lastClickTime > clickTimeout) {
        clickCount = 0; // Si pasa mucho tiempo, reiniciamos
      }
      lastClickTime = now;
      clickCount++;

      // 🚨 Verificación de emergencia no activa
      if (clickCount == 2) {
        if (!emergenciaActiva) {
          Serial.println("[Botón] 2 clics detectados ➔ Activando emergencia");
          if (uxQueueSpacesAvailable(xQueueEventos) > 0) {
            xQueueSend(xQueueEventos, (void *)"ACTIVAR", portMAX_DELAY);
          }
        } else {
          Serial.println("[Botón] 2 clics detectados, pero emergencia YA activa. Ignorando.");
        }
        clickCount = 0; // Reiniciar conteo después de manejar
      }
    }

    if (emergencyBtn.read() == LOW) {
      if (!buttonHeld) {
        holdStart = millis();
        buttonHeld = true;
      }
      else if (millis() - holdStart > 4000) {
        if (emergenciaActiva) {
          Serial.println("[Botón] Botón mantenido 4s ➔ Cancelando emergencia");
          if (uxQueueSpacesAvailable(xQueueEventos) > 0) {
            xQueueSend(xQueueEventos, (void *)"CANCELAR", portMAX_DELAY);
          }
        } else {
          Serial.println("[Botón] Mantuvimos 4s, pero NO había emergencia activa. Ignorando.");
        }
        buttonHeld = false;
        clickCount = 0;
        vTaskDelay(500 / portTICK_PERIOD_MS); // Pequeño delay para no repetir
      }
    } else {
      buttonHeld = false;
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);  // Evitar rebotes
  }
}




void taskLEDs(void *param) {
  const unsigned long intervaloParpadeo = 250;
  unsigned long ultimaActualizacion = 0;
  bool estadoLED = false;
  
  // Variables para control de tiempos
  unsigned long ultimoCambioGSM = 0;
  unsigned long ultimoCambioBLE = 0;
  bool estadoGSM = false;
  bool estadoBLE = false;

  for (;;) {
    unsigned long ahora = millis();

    // LED_GSM - Parpadeo lento (2s) para error GSM/SIM
    if (errorGSM) {
      if (ahora - ultimoCambioGSM >= 1000) { // Cambia cada 1 segundo (2s ciclo completo)
        estadoGSM = !estadoGSM;
        digitalWrite(LED_GSM, estadoGSM);
        ultimoCambioGSM = ahora;
      }
    }
    // LED_GSM - Comportamiento normal (llamadas)
    // Dentro del bucle de taskLEDs
if (errorGSM) {
  // Error GSM: parpadeo lento
  if (ahora - ultimoCambioGSM >= 1000) {
    estadoGSM = !estadoGSM;
    digitalWrite(LED_GSM, estadoGSM);
    ultimoCambioGSM = ahora;
  }
} 
else if (llamadaActiva) {
  // Llamada activa: parpadeo rápido
  if (ahora - ultimoCambioGSM >= 500) {
    estadoGSM = !estadoGSM;
    digitalWrite(LED_GSM, estadoGSM);
    ultimoCambioGSM = ahora;
  }
} 
else {
  // 🔥 Estado normal: LED apagado garantizado
  estadoGSM = false;
  digitalWrite(LED_GSM, LOW);
}


    // LED_DATA - Parpadeo rápido (0.2s) para BLE activo
    if (bleActivo) {
      if (ahora - ultimoCambioBLE >= 200) { // Cambia cada 200ms
        estadoBLE = !estadoBLE;
        digitalWrite(LED_DATA, estadoBLE);
        ultimoCambioBLE = ahora;
      }
    }
    
    if (bleActivo && (millis() - bleActivatedTime > bleInactiveTimeout)) {
  bleActivo = false;
  modoConfiguracionBLE = false;
  Serial.println("BLE desactivado por inactividad");
}

    // LED_DATA - Comportamientos existentes
    else if (parpadeoEmergenciaActivo && ahora - ultimaActualizacion > intervaloParpadeo) {
      estadoLED = !estadoLED;
      digitalWrite(LED_DATA, estadoLED);
      ultimaActualizacion = ahora;
    }
    else if (smsEnviadoOK) {
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_DATA, HIGH); delay(150);
        digitalWrite(LED_DATA, LOW); delay(150);
      }
      smsEnviadoOK = false;
    }
    else if (smsEnviadoFallo) {
      for (int i = 0; i < 6; i++) {
        digitalWrite(LED_DATA, HIGH); delay(100);
        digitalWrite(LED_DATA, LOW); delay(100);
      }
      smsEnviadoFallo = false;
    }

    

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}



void taskBotonBLE(void *param) {
  Bounce bleBtn = Bounce();
  bleBtn.attach(BTN_BLE);
  bleBtn.interval(25);

  int bleClicks = 0;
  unsigned long lastClickTime = 0;

  for (;;) {
    bleBtn.update();

    if (bleBtn.fell()) {
      unsigned long now = millis();
      if (now - lastClickTime > BLE_CLICK_TIMEOUT) bleClicks = 0;
      lastClickTime = now;
      bleClicks++;

if (emergenciaActiva) {
  Serial.println("❌ No se puede activar BLE durante emergencia.");
  bleClicks = 0;
  continue; // Ignorar
}


      if (bleClicks >= BLE_CLICK_COUNT && !modoConfiguracionBLE) {
        modoConfiguracionBLE = true;
        Serial.println("🔧 Activando BLE (6 clics detectados)");
        iniciarBLE();
        bleClicks = 0;
      }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}


void taskEventos(void *param) {
  char buffer[16];
  for (;;) {
    if (xQueueReceive(xQueueEventos, &buffer, portMAX_DELAY)) {
      if (String(buffer) == "ACTIVAR" && !emergenciaActiva) {
        emergenciaActiva = true;
        llamadaRealizada = false;
        lastSMSTime = millis();
        enviarSMS();
        hacerLlamada();
        digitalWrite(LED_DATA, HIGH);
        parpadeoEmergenciaActivo = true;

      }
      if (String(buffer) == "CANCELAR" && emergenciaActiva) {
        emergenciaActiva = false;
llamadaRealizada = false;
parpadeoEmergenciaActivo = false;
smsEnviadoOK = false;
smsEnviadoFallo = false;

if (xSemaphoreTake(xMutexGSM, pdMS_TO_TICKS(3000))) {
  modem.callHangup();
  xSemaphoreGive(xMutexGSM);
} else {
  Serial.println("⚠️ No se pudo tomar GSM para colgar durante cancelación");
}

llamadaActiva = false;
digitalWrite(LED_DATA, LOW);
digitalWrite(LED_GSM, LOW);
Serial.println("🛑 Emergencia cancelada.");


      }
    }
  }
}

void taskGSM(void *param) {
  for (;;) {
    // Decodificar datos del GPS
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }

    // Enviar SMS periódicamente durante la emergencia
    if (emergenciaActiva && millis() - lastSMSTime >= SMS_INTERVAL) {
      Serial.println("⏳ Intervalo alcanzado. Enviando SMS de seguimiento...");
      enviarSMS();  // Esta función ya tiene la lógica de reintentos
      lastSMSTime = millis();
    }

    delay(100);  // Espera para liberar CPU
  }
}


void taskEmergencia(void *parameter);  // 👈 Declaración de la tarea


void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  sim800.begin(GSM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
  xMutexGSM = xSemaphoreCreateMutex();



  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_DATA, OUTPUT);
  pinMode(LED_GSM, OUTPUT);
  pinMode(BTN_EMERGENCY, INPUT_PULLUP);
  pinMode(BTN_BLE, INPUT_PULLUP);
  digitalWrite(LED_POWER, HIGH);

  emergencyBtn.attach(BTN_EMERGENCY);
  emergencyBtn.interval(25);

  if (!FFat.begin()) Serial.println("Error FFat");
  else cargarDatosDesdeFS();

// ➡️ Solo intentar iniciar el GSM 1 vez y seguir, SIN BLOQUEAR:
Serial.println("Inicializando módem GSM (sin RST)...");
errorGSM = false; // Asumir que funciona hasta probar lo contrario

// 1. Intento de reinicio por software
sim800.println("AT+CFUN=1,1"); // Reset por software
delay(5000); // Espera crítica para reinicio

// 2. Verificar si el módem responde
sim800.println("AT");
if (modem.waitResponse(2000) != 1) {
  Serial.println("❌ Módem no responde a comandos AT");
  errorGSM = true;
} 
else if (!modem.init()) { // Reemplaza modem.restart() con esta verificación
  Serial.println("❌ Fallo en inicialización del módem");
  errorGSM = true;
} 
else {
  // ----- Verificación de tarjeta SIM -----
  Serial.print("Estado SIM: ");
  int simStatus = modem.getSimStatus();
  if (simStatus != 3) { // 3 = SIM_READY en TinyGSM
    Serial.println("❌ Error en SIM (Código: " + String(simStatus) + ")");
    errorGSM = true;
  } 
  else {
    Serial.println("OK");
    // ----- Verificación de red -----
    if (modem.waitForNetwork(30000L)) {
      // Configurar modo texto para SMS
      modem.sendAT("+CMGF=1");
      if (modem.waitResponse(5000L) == 1) {  // Esperar hasta 5 segundos por "OK"
        Serial.println("✅ GSM listo (modo SMS configurado)");
      } 
      else {
        Serial.println("⚠️ GSM: Error configurando modo SMS");
        errorGSM = true;
      }
    } 
    else {
      Serial.println("⚠️ No se encontró red GSM");
      errorGSM = true;
    }
  }
}

  // Ahora sí, creamos las tareas:
  xQueueEventos = xQueueCreate(5, sizeof(char[16]));
  xTaskCreatePinnedToCore(taskBoton, "Boton", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskEventos, "Eventos", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskGSM, "GSM", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskBotonBLE, "BotonBLE", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskLEDs, "LEDs", 2048, NULL, 1, NULL, 0);
}


void loop() {
  // No se usa porque todo está manejado por FreeRTOS
}

