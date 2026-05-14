#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

// --- CONFIGURACIÓN WIFI ---
const char* ssid = "iPhone de Axhel Giovanni";
const char* password = "987654312.";

// IMPORTANTE: Cambia "TU_IP_LOCAL" por la IP de tu PC (ej. 192.168.1.75)
// El puerto 3307 lo maneja PHP internamente, aquí solo llamamos al archivo.
const char* serverName = "http://172.20.10.5/arcade_handler.php"; 

// --- CONFIGURACIÓN HARDWARE ---
#define SDA_PIN 21
#define SCL_PIN 22
#define PIN_MAQUINARIA 2 // LED interno que simula el estado del juego

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

void setup(void) {
  Serial.begin(115200);
  pinMode(PIN_MAQUINARIA, OUTPUT);
  digitalWrite(PIN_MAQUINARIA, LOW);

  // Conexión WiFi
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado exitosamente");

  // Inicio de Sensor NFC
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("No se encontró el sensor PN532");
    while (1);
  }
  nfc.SAMConfig();
  Serial.println("SISTEMA ARCADE: ESPERANDO TARJETA PARA INICIAR...");
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  // Espera a detectar una tarjeta
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("\n--- TARJETA DETECTADA ---");
    
    // 1. Obtener el UID en formato Hexadecimal para el servidor
    String hexStr = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) hexStr += "0";
      hexStr += String(uid[i], HEX);
    }
    hexStr.toUpperCase();

    // 2. Mostrar bases en el Monitor Serial (Requisito de la práctica)
    imprimirBases(uid, uidLength);

    // 3. Enviar datos al servidor PHP (MySQL puerto 3307)
    enviarAlServidor(hexStr);

    // 4. Feedback visual (Parpadeo de LED)
    digitalWrite(PIN_MAQUINARIA, HIGH);
    delay(500);
    digitalWrite(PIN_MAQUINARIA, LOW);
    
    delay(2500); // Pausa para evitar lecturas dobles
    Serial.println("\nEsperando siguiente jugador...");
  }
}

void enviarAlServidor(String uid) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Iniciar conexión con el archivo puente PHP
    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Preparamos los datos para registrar_inicio
    String httpRequestData = "registrar_inicio=1&uid=" + uid;
    
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      Serial.print("Servidor respondió: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println("Mensaje: " + payload);
    } else {
      Serial.print("Error en petición HTTP: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Desconectado");
  }
}

void imprimirBases(uint8_t *uid, uint8_t uidLength) {
  // Hexadecimal
  Serial.print("HEX: ");
  for (uint8_t i = 0; i < uidLength; i++) {
    Serial.print(uid[i], HEX); Serial.print(" ");
  }
  
  // Decimal
  unsigned long dec = 0;
  for (uint8_t i = 0; i < uidLength; i++) dec = (dec << 8) | uid[i];
  Serial.print(" | DEC: "); Serial.print(dec);

  // Binario
  Serial.print(" | BIN: ");
  for (int i = 0; i < uidLength; i++) {
    for (int bit = 7; bit >= 0; bit--) Serial.print(bitRead(uid[i], bit));
    Serial.print(" ");
  }
  Serial.println();
}