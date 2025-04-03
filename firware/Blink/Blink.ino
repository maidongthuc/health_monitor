#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 

// Thông tin WiFi và MQTT
const char* ssid = "C305";
const char* password = "Spkt2020";
// const char* ssid = "Co Nhu";
// const char* password = "conhu123";

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
  float temp = -1.0;  // Giá trị mặc định là -1 để biểu thị chưa có dữ liệu
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

const unsigned long TIMEOUT_WIFI = 30000; // Thời gian chờ kết nối WiFi

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi() {
  // Nếu đã kết nối, không cần làm gì
  if (WiFi.status() == WL_CONNECTED) {
    if (lastWifiStatus != CONNECTED) {
      Serial.println("OKWIFI");
      lastWifiStatus = CONNECTED;
    }
    return;
  }

  if (lastWifiStatus != CONNECTING) {
    Serial.println("NOWIFI");
    lastWifiStatus = CONNECTING;
  }

  WiFi.begin(ssid, password);
  unsigned long startMillis = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMillis < TIMEOUT_WIFI) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (lastWifiStatus != CONNECTED) {
      Serial.println("OKWIFI");
      lastWifiStatus = CONNECTED;
    }
  } else {
    if (lastWifiStatus != DISCONNECTED) {
      Serial.println("NOWIFI");
      lastWifiStatus = DISCONNECTED;
    }
  }
}

// Kết nối lại MQTT
void reconnect() {
    while (!client.connected()) {
        // Serial.print("Attempting MQTT connection...");
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
  if(strcmp(topic, "Livingroom/device_1") != 0){
    JsonDocument doc;
    doc["topic"] = topic;
    char message[length + 1]; // +1 cho ký tự null
    memcpy(message, payload, length);
    message[length] = '\0'; // Thêm ký tự kết thúc chuỗi
    doc["msg"] = message;
    String jsonOutput;
    serializeJson(doc, jsonOutput);
    Serial.println(jsonOutput);
  }
}

// Gửi dữ liệu qua MQTT
void sendData(String topic, String message) {
    client.publish(topic.c_str(), message.c_str());
}

// Khởi tạo
void setup() {
    Serial.begin(9600);
    while (!Serial) delay(1);
    setup_wifi();
    espClient.setInsecure(); // Cảnh báo: Không an toàn trong thực tế
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

// Gửi dữ liệu lên server nếu có thay đổi
void sendMQTT(HealthData data) {
  static HealthData last_data;
  JsonDocument doc;
  String postData;

  if (data.temp != last_data.temp && data.spo2 != last_data.spo2 && data.hr != last_data.hr &&
      data.sys != last_data.sys && data.dia != last_data.dia) 
  { 
    doc["temp"] = data.temp;
    doc["spo2"] = data.spo2;
    doc["hr"] = data.hr;
    doc["sys"] = data.sys;
    doc["dia"] = data.dia;
    serializeJson(doc, postData);
    sendData(TOPIC_1, postData);
    last_data = data;
  }
}

// Vòng lặp chính
void loop() {
  // Kiểm tra kết nối WiFi và MQTT
  if (WiFi.status() != WL_CONNECTED) setup_wifi();
  if (!client.connected()) reconnect();
  client.loop();
  // Nhận dữ liệu từ Serial
  JsonDocument doc;
  if (Serial.available()) {
    String receivedData = Serial.readStringUntil('\n');
    receivedData.trim();
    Serial.println(receivedData);

    DeserializationError error = deserializeJson(doc, receivedData);
    if (error) {
      Serial.print("deserializeJson() returned ");
      Serial.println(error.c_str());
      return;
    }
    current_data.temp =  doc["temp"];
    current_data.spo2 = doc["spo2"];
    current_data.hr = doc["hr"];
    current_data.sys = doc["sys"];
    current_data.dia = doc["dia"];
  }
  sendMQTT(current_data);
}