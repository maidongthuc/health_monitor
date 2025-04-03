#include <Wire.h>
// #include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 8 // Chân nối với Arduino
#define NUM_SAMPLES 10 // Số mẫu dùng để tính trung bình trượt

OneWire oneWire(ONE_WIRE_BUS); //Thiết đặt thư viện onewire
DallasTemperature sensors(&oneWire);  // Truyền thông tin cảm biến đến DallasTemperature library
// LiquidCrystal_I2C lcd(0x27, 16, 2); // Thiết đặt màn hình LCD I2C địa chỉ 0x27, kích thước 16x2

float temperatureSamples[NUM_SAMPLES];
int sampleIndex = 0;
bool bufferFilled = false;

float calibrationFactor = 1.21; // Hệ số hiệu chỉnh
float offset = -6.05;             // Offset

// Hàm hiệu chỉnh nhiệt độ
float calibrateTemperature(float temperature) {
  return (temperature * calibrationFactor) + offset;
}

void setup() {
  Serial.begin(9600);
  sensors.begin();
  // lcd.init(); // Khởi tạo màn hình LCD
  // lcd.backlight(); // Bật đèn nền màn hình LCD
}

void loop() {
  sensors.requestTemperatures(); // Yêu cầu cảm biến đo nhiệt độ
  float temperatureC = sensors.getTempCByIndex(0);  // Lấy giá trị nhiệt độ từ cảm biến
  float calibratedTemperature = calibrateTemperature(temperatureC); // Hiệu chỉnh giá trị nhiệt độ

  // Lưu giá trị nhiệt độ vào mảng mẫu
  temperatureSamples[sampleIndex] = calibratedTemperature;
  sampleIndex++;

  if (sampleIndex >= NUM_SAMPLES) {
    sampleIndex = 0;
    bufferFilled = true;
  }

  // Tính trung bình trượt
  float smoothedTemperature = 0.0;
  int numSamples = bufferFilled ? NUM_SAMPLES : sampleIndex;
  for (int i = 0; i < numSamples; i++) {
    smoothedTemperature += temperatureSamples[i];
  }
  smoothedTemperature /= numSamples;

  // // Hiển thị giá trị nhiệt độ trung bình trên LCD
  // lcd.clear();
  // lcd.setCursor(0, 0);
  // lcd.print("Nhiet do tb:");
  // lcd.setCursor(0, 1);
  // lcd.print(smoothedTemperature);
  // lcd.print(" C");

  // Gửi giá trị nhiệt độ trung bình qua Serial 2
  Serial.print("<TEMP:");
  Serial.print(smoothedTemperature);
  Serial.println(">");

  delay(1000);
}
