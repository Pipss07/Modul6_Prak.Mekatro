#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ===== KONFIGURASI WIFI =====
const char* ssid     = "FCP";
const char* password = "laridulu5x";

// ===== BROKER BARU: HIVEMQ =====
// HiveMQ Public Broker sangat stabil untuk testing
const char* mqtt_server = "broker.hivemq.com"; 
const int mqtt_port = 1883;

// ===== TOPIC =====
const char* topic_led  = "esp8266/led";
const char* topic_temp = "esp8266/temp";
const char* topic_hum  = "esp8266/hum";

// ===== PIN =====
#define LED_PIN 2        // D4 (Internal LED)
#define DHT_PIN 14       // D5
#define DHT_TYPE DHT11

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHT_TYPE);

unsigned long lastPublish = 0;
const unsigned long interval = 5000; 

void setup_wifi() {
  delay(10);
  Serial.println("\n------------------------------");
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.printf("Pesan Masuk [%s]: %s\n", topic, message.c_str());

  if (String(topic) == topic_led) {
    if (message == "ON") {
      digitalWrite(LED_PIN, LOW); // LED ON
    } else if (message == "OFF") {
      digitalWrite(LED_PIN, HIGH); // LED OFF
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Mencoba koneksi MQTT ke HiveMQ...");
    
    // Client ID acak agar tidak bentrok
    String clientId = "NodeMCU-Project-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("BERHASIL TERHUBUNG!");
      client.subscribe(topic_led);
    } else {
      Serial.print("GAGAL, rc=");
      Serial.print(client.state());
      Serial.println(" Coba lagi dalam 5 detik...");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.begin(115200);
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastPublish >= interval) {
    lastPublish = now;

    float hum  = dht.readHumidity();
    float temp = dht.readTemperature();

    if (isnan(hum) || isnan(temp)) {
      Serial.println("Sensor Error: Cek kabel DHT11!");
      return;
    }

    char tempStr[8];
    char humStr[8];
    dtostrf(temp, 1, 2, tempStr);
    dtostrf(hum, 1, 2, humStr);

    client.publish(topic_temp, tempStr);
    client.publish(topic_hum, humStr);

    Serial.printf("Kirim -> Suhu: %s C | Lembab: %s %%\n", tempStr, humStr);
  }
}