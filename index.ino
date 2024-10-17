const int gasPin = 32;
const int buzzerPin = 27;
const int ledRedPin = 25;    // Pin para el LED rojo
const int ledGreenPin = 26;  // Pin para el LED verde

#define BLYNK_TEMPLATE_ID "TMPL2benw0x3p"
#define BLYNK_TEMPLATE_NAME "Unidad 1"
#define BLYNK_AUTH_TOKEN "LULfH79rr_GZ5Y0HbuJXIq5iTYVOxVE9"

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "WIFI-DCI";
const char* password = "DComInf_2K24";

void setup() {
  // Inicializar la comunicación serial
  Serial.begin(9600);

  // Configurar los pines del sensor de gas, buzzer, y LEDs
  pinMode(gasPin, INPUT);
  pinMode(buzzerPin, OUTPUT);  
  pinMode(ledRedPin, OUTPUT);   
  pinMode(ledGreenPin, OUTPUT);

  // Asegurarse de que el buzzer y los LEDs estén apagados al inicio
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledRedPin, LOW);
  digitalWrite(ledGreenPin, HIGH);  // El LED verde se enciende al inicio (todo está bien)

  // Conectar a la red WiFi
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");

  // Inicializar Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, "WIFI-DCI", "DComInf_2K24");
}

void loop() {

  int gasValue = analogRead(gasPin);
  Serial.print("Valor del detector de gas: ");
  Serial.println(gasValue);
  Blynk.run();
  Blynk.virtualWrite(V5, gasValue);

  if (gasValue > 1000) {
    Serial.println("Blynk event: CO alto");
    Blynk.logEvent("event"); //emitir evento de Blynk

    // Activar el buzzer y encender el LED rojo
    tone(buzzerPin, 1000);
    digitalWrite(ledRedPin, HIGH);   // Encender LED rojo
    digitalWrite(ledGreenPin, LOW);  // Apagar LED verde

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin("http://172.27.129.146:3000/check");  // URL de la petición

      http.addHeader("Content-Type", "application/json");  // Especificar el tipo de contenido

      // Crear el cuerpo JSON con gasValue y message
      String postData = "{\"gasValue\":" + String(gasValue) + ", \"message\":\"CO alto detectado\"}";

      int httpResponseCode = http.POST(postData);  // Realizar la solicitud POST

      // Verificar el código de respuesta HTTP
      if (httpResponseCode > 0) {
        Serial.print("Código de respuesta HTTP: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println("Respuesta: " + payload);
      } else {
        Serial.print("Error en la solicitud POST: ");
        Serial.println(httpResponseCode);
      }

      http.end();  // Cerrar la conexión
    } else {
      Serial.println("WiFi desconectado");
    }
  } else {
    // Apagar el buzzer, apagar LED rojo y encender LED verde
    noTone(buzzerPin);
    digitalWrite(ledRedPin, LOW);    // Apagar LED rojo
    digitalWrite(ledGreenPin, HIGH); // Encender LED verde (todo está bien)
  }
  
  delay(5000);
}
