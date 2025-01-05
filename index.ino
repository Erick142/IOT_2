#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>

// Configuración WiFi y MQTT
#define WIFI_AP "WIFI-DCI"
#define WIFI_PASSWORD "DComInf_2K24"
#define MQTT_SERVER "iot.ceisufro.cl" 
#define MQTT_PORT 1883
#define MQTT_TOPIC "v1/devices/me/telemetry"
#define TOKEN "XzhWemx4shJmrk30DDpo"

// Configuración de dispositivo
const int deviceId = 7;
const char* SERVER_URL = "http://172.27.91.64:3000/device";

// Pines
const int gasPin = 32;
const int buzzerPin = 27;
const int ledRedPin = 25;   
const int ledGreenPin = 26; 

// Objetos WiFi y MQTT
WiFiClient wifiClient;
PubSubClient client(wifiClient);
HTTPClient http;

unsigned long lastSend = 0;
unsigned long lastCheck = 0;
bool deviceCreated = false;

void setup() {
  Serial.begin(115200);

  // Configurar pines
  pinMode(gasPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);

  // Configurar el estado inicial de los pines
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledRedPin, LOW);
  digitalWrite(ledGreenPin, HIGH);

  // Conectar a la red WiFi
  connectToWiFi();

  // Conectar al servidor MQTT
  connectToMQTT();

  // Intentar obtener o crear el dispositivo
  if (!fetchDevice(deviceId)) {
    deviceCreated = createDevice("Dispositivo de prueba 10");
  }

  lastSend = 0;
  lastCheck = millis();
}

void loop() {
  // Reconexion al MQTT si es necesario
  if (!client.connected()) {
    reconnectMQTT();
  }

  // Enviar datos de gas cada 2.5 segundos
  unsigned long currentMillis = millis();
  if (currentMillis - lastSend >= 2500) {
    sendGasData();
    lastSend = currentMillis;
  }

  // Verificar el estado activo del dispositivo cada 2.5 segundos
  if (currentMillis - lastCheck >= 2500) {
    fetchDeviceActiveStatus(deviceId);
    lastCheck = currentMillis;
  }

  // Ejecutar loop de MQTT
  client.loop();

  delay(10);
}

// Función para conectar a WiFi
void connectToWiFi() {
  Serial.println("Conectando a WiFi...");
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
}

// Función para conectar a MQTT
void connectToMQTT() {
  Serial.println("Conectando al servidor MQTT...");
  client.setServer(MQTT_SERVER, MQTT_PORT);

  while (!client.connected()) {
    Serial.print("Conectando a MQTT...");
    if (client.connect("ESP32_Device", TOKEN, NULL)) {
      Serial.println("Conectado a MQTT");
    } else {
      Serial.print("Error de conexión, código de estado: ");
      Serial.println(client.state());
      delay(1000);
    }
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Reconectando a MQTT...");
    if (client.connect("ESP32_Device", TOKEN, NULL)) {
      Serial.println("Reconectado a MQTT");
    } else {
      Serial.print("Error de reconexión, código de estado: ");
      Serial.println(client.state());
      delay(1000);
    }
  }
}

// Función para enviar datos de gas
void sendGasData() {
  int gasValue = analogRead(gasPin);
  Serial.print("Valor del detector de gas: ");
  Serial.println(gasValue);

  // Preparar el payload en formato JSON
  String payload = "{\"gasValue\":" + String(gasValue) + "}";
  client.publish(MQTT_TOPIC, payload.c_str());
  Serial.println("Datos enviados a MQTT: " + payload);

  // Comportamiento en caso de detección de gas
  if (gasValue > 800) {
    Serial.println("CO alto detectado");
    tone(buzzerPin, 1000);
    digitalWrite(ledRedPin, HIGH);
    digitalWrite(ledGreenPin, LOW);

    notifyHighGasLevel(deviceId, gasValue);
  } else {
    noTone(buzzerPin);
    digitalWrite(ledRedPin, LOW);
    digitalWrite(ledGreenPin, HIGH);
  }
}

// Función para obtener el dispositivo
bool fetchDevice(int deviceId) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = String(SERVER_URL) + "/" + deviceId;
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      Serial.println("Dispositivo encontrado.");
      http.end();
      return true;
    } else {
      Serial.println("Error al obtener el dispositivo. Código: " + String(httpResponseCode));
      http.end();
      return false;
    }
  } else {
    Serial.println("Error: WiFi no conectado.");
    return false;
  }
}

// Función para crear el dispositivo
bool createDevice(const char* name) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = String(SERVER_URL);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"name\":\"" + String(name) + "\"}";
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode == 200) {
      Serial.println("Dispositivo creado exitosamente.");
      http.end();
      return true;
    } else {
      Serial.println("Error al crear el dispositivo. Código: " + String(httpResponseCode));
      http.end();
      return false;
    }
  } else {
    Serial.println("Error: WiFi no conectado.");
    return false;
  }
}

// Función para verificar el estado activo del dispositivo
void fetchDeviceActiveStatus(int deviceId) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = String(SERVER_URL) + "/" + deviceId + "/active";
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      Serial.println("Estado activo del dispositivo recibido.");
    } else {
      Serial.println("Error al obtener estado activo. Código: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("Error: WiFi no conectado.");
  }
}

void notifyHighGasLevel(int id, int gasValue) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = String(SERVER_URL) + "/" + id + "/gas/" + gasValue;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(""); // Enviar petición POST sin cuerpo

    if (httpResponseCode == 200) {
      Serial.println("Notificación de gas enviada exitosamente.");
    } else {
      Serial.println("Error al enviar notificación de gas. Código: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("Error: WiFi no conectado.");
  }
}
