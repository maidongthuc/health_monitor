#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 

// Thông tin MQTT
const char* mqtt_server = "42cb8a3135f84357959ce239305850c0.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "smarthome";
const char* mqtt_password = "Smarthome2023";

// const char* mqtt_server = "1df19fa858774630a1197a48081cc0c1.s1.eu.hivemq.cloud";
// const int mqtt_port = 8883;
// const char* mqtt_username = "chechanh2003";
// const char* mqtt_password = "0576289825Asd";

#define TOPIC_1 "Livingroom/device_1"
#define TOPIC_2 "chatbot"

// Cấu trúc dữ liệu sức khỏe
struct HealthData {
  float temp = -1.0;
  int spo2 = -1;
  int hr = -1;
  int sys = -1;
  int dia = -1;
};
enum WifiStatus { 
  DISCONNECTED, 
  CONNECTING, 
  CONNECTED 
};

WifiStatus lastWifiStatus = DISCONNECTED;
HealthData current_data;

const unsigned long TIMEOUT_WIFI = 10000; // Timeout 10 giây

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (lastWifiStatus != CONNECTED) {
      Serial.println("OKWIFI");
      lastWifiStatus = CONNECTED;
    }
    return;
  }

  if (lastWifiStatus != CONNECTING) {
    // Serial.println("NOWIFI");
    lastWifiStatus = CONNECTING;
  }

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(TIMEOUT_WIFI / 100);
  if (!wifiManager.autoConnect("Health Monitor")) {
    if (lastWifiStatus != DISCONNECTED) {
      // Serial.println("NOWIFI");
      lastWifiStatus = DISCONNECTED;
    }
    return; 
  }

  // Khi kết nối thành công
  if (WiFi.status() == WL_CONNECTED) {
    if (lastWifiStatus != CONNECTED) {
      Serial.println("OKWIFI");
      // Serial.print("Connected to: ");
      // Serial.println(WiFi.SSID());
      // Serial.print("IP Address: ");
      // Serial.println(WiFi.localIP());
      lastWifiStatus = CONNECTED;
    }
  }
}

// Callback khi vào chế độ cấu hình WiFi
void configModeCallback(WiFiManager *myWiFiManager) {
  // Serial.println("NOWIFI"); // In NOWIFI khi vào chế độ cấu hình
  // Serial.println("Entered config mode");
  // Serial.println("Please connect to WiFi: ");
  // Serial.println(myWiFiManager->getConfigPortalSSID());
  // Serial.println(WiFi.softAPIP());
}

// Kết nối lại MQTT
void reconnect() {
  while (!client.connected()) {
    String clientID = "ESPClient-" + String(random(0xffff), HEX);
    if (client.connect(clientID.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("mqtt connected");
      client.subscribe(TOPIC_1);
      client.subscribe(TOPIC_2);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Callback khi nhận tin nhắn MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "Livingroom/device_1") != 0) {
    StaticJsonDocument<256> doc;
    doc["topic"] = topic;
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    doc["msg"] = message;
    String jsonOutput;
    serializeJson(doc, jsonOutput);
    Serial.println(jsonOutput);
  }
}

// Gửi dữ liệu qua MQTT
void sendData(String topic, String message) {
  if (client.connected()) {
    client.publish(topic.c_str(), message.c_str());
  }
}

// Khởi tạo
void setup() {
  Serial.begin(9600);
  while (!Serial) delay(1);
  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// Gửi dữ liệu lên server nếu có thay đổi
void sendMQTT(HealthData data) {
  static HealthData last_data;
  if (!client.connected()) return;
  if (data.temp != last_data.temp && data.spo2 != last_data.spo2 && 
      data.hr != last_data.hr && data.sys != last_data.sys && data.dia != last_data.dia) {
    JsonDocument doc;
    doc["temp"] = data.temp;
    doc["spo2"] = data.spo2;
    doc["hr"] = data.hr;
    doc["sys"] = data.sys;
    doc["dia"] = data.dia;
    String postData;
    serializeJson(doc, postData);
    sendData(TOPIC_1, postData);
    last_data = data;
  }
}

// Vòng lặp chính
void loop() {
  if (WiFi.status() != WL_CONNECTED) setup_wifi();
  if (!client.connected() && WiFi.status() == WL_CONNECTED) reconnect();
  client.loop();

  if (Serial.available()) {
    String receivedData = Serial.readStringUntil('\n');
    receivedData.trim();
    // Serial.println(receivedData);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, receivedData);
    if (error) {
      Serial.println(error.c_str());
      return;
    }
    current_data.temp = doc["temp"];
    current_data.spo2 = doc["spo2"];
    current_data.hr = doc["hr"];
    current_data.sys = doc["sys"];
    current_data.dia = doc["dia"];
  }
  sendMQTT(current_data);
}