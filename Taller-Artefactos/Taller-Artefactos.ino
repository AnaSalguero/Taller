#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

const char* WIFI_SSID = "hi";
const char* WIFI_PASSWORD = "salitagua";


// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================


const char* MQTT_SERVER = "172.17.26.27";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";


// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================
#define DHTPIN 4
#define DHTTYPE DHT11
#define LDR 32

DHT dht(DHTPIN, DHTTYPE);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;

//variables
float temperatura = 0;
float humedad = 0;
float luz = 0;
float lectura = 0;

void conectarWiFi() {

  Serial.print("Conectando a WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println(" conectado");

    } else {

      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");

      delay(2000);
    }
  }
}


// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {

  temperatura = dht.readTemperature();
  humedad = dht.readHumidity();
  lectura = analogRead(LDR);
  luz = (lectura/4095)*100; 

  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error en los datos del sensor DHT11");
    return;
  }
  char temp[10];
  char hum[10];
  char l[10];

  dtostrf(temperatura, 6, 2, temp);
  dtostrf(humedad, 6, 2, hum);
  dtostrf(luz, 6, 2, l);
    
  mqttClient.publish("clima/esp32/temperatura", temp);
  mqttClient.publish("clima/esp32/humedad", hum);
  mqttClient.publish("clima/esp32/luz", l);
  
  Serial.print("Temperatura: ");
  Serial.println(temperatura);
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println("%");
  Serial.print("luz: ");
  Serial.print(luz);
  Serial.println("%");

}

void setup() {

  Serial.begin(115200);

  pinMode(32,OUTPUT);
  dht.begin();

  conectarWiFi();

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );
}

void loop() {

  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();


  // Publicar sensores cada 2 segundos.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {

    ultimoEnvio = millis();

    publicarSensores();
  }
}