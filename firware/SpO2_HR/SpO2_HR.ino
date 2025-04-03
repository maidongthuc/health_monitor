#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

unsigned long startMillis;
unsigned long currentMillis;

PulseOximeter pox;
uint32_t tsLastReport = 0;
#define REPORTING_PERIOD_MS 100
unsigned long sendDataInterval = 2000; // Gửi dữ liệu mỗi 1 giây
unsigned long lastSendTime = 0; // Lưu thời điểm lần cuối gửi dữ liệu
// SpO2
//#define BUFFER_SIZE 49
#define BUFFER_SIZE 21
float spo2Values[BUFFER_SIZE];
int spo2Index = 0;
bool bufferFullspo2 = false;
int discardCount = 0;
#define DISCARD_LIMIT 10
#define CALIBRATION_FACTOR 1.0189
#define OFFSETspo2 0.2892 

// HR
int validReadingsCount = 0;
const int skipCount = 10;
const int numSamplesHR = 20;
float heartRateSamples[numSamplesHR];
int sampleIndexHR = 0;
bool bufferFilledHR = false;
#define CALIBRATION_FACTOR_HR 1.0133  // Hệ số hiệu chỉnh từ phân tích
#define OFFSET_HR -0.0135  // Offset từ phân tích 

float medianSpo2 = 0.0;
float avgHeartRate = 0.0;

//LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  // lcd.init();
  // lcd.backlight();
  //Serial.print("Initializing pulse oximeter..");
  bool initialized = false;
  int maxAttempts = 5;
  int attempt = 0;
  while (!initialized && attempt < maxAttempts) {
    if (pox.begin()) {
      initialized = true;
      //Serial.println("SUCCESS");
      pox.setOnBeatDetectedCallback(onBeatDetected);
    } else {
      attempt++;
      delay(1000); // Đợi 1 giây trước khi thử lại
    }
  }
  if (!initialized) {
    //Serial.println("Failed to initialize pulse oximeter after multiple attempts.");
    for(;;); // Vòng lặp vô hạn nếu khởi tạo thất bại
  }
}

void loop() {    
  uint32_t currentMillis2 = millis();
  pox.update();
  if (currentMillis2 - tsLastReport > REPORTING_PERIOD_MS) {
    tsLastReport = currentMillis2;

    float spo2 = pox.getSpO2();
    float heartRate = pox.getHeartRate();
    if ( (spo2 <= 0) || (heartRate <= 0) ) {
      resetBuffer();
      resetHeartRateBuffer();
    } else {
      // Hệ số điều chỉnh
      spo2 = spo2 * CALIBRATION_FACTOR + OFFSETspo2;
      heartRate = heartRate * CALIBRATION_FACTOR_HR + OFFSET_HR;
    }
    if (spo2 >= 70 && spo2 <= 100) {
      if (discardCount < DISCARD_LIMIT) discardCount++;
      else {
        // Lưu giá trị vào bộ đệm, thay thế giá trị cũ nhất nếu bộ đệm đầy
        spo2Values[spo2Index] = spo2;
        spo2Index = (spo2Index + 1) % BUFFER_SIZE;
        if (spo2Index == 0) bufferFullspo2 = true;
        // Tính trung vị nếu bộ đệm đầy
        if (bufferFullspo2) {
          medianSpo2 = calculateMedian(spo2Values, BUFFER_SIZE);
          if (heartRate >= 60 && heartRate <= 100) {
            if (bufferFilledHR) {
              heartRateSamples[0] = heartRate;
            } else {
              heartRateSamples[sampleIndexHR] = heartRate;
              sampleIndexHR = (sampleIndexHR + 1) % numSamplesHR;
              if (sampleIndexHR == 0) bufferFilledHR = true;
            }
            avgHeartRate = 0;
            int count = bufferFilledHR ? numSamplesHR : sampleIndexHR;
            for (int i = 0; i < count; i++) avgHeartRate += heartRateSamples[i];
            avgHeartRate /= count;
            if (validReadingsCount < skipCount) validReadingsCount++;
            // Gửi dữ liệu qua Serial mỗi 1 giây
            if (currentMillis2 - lastSendTime >= sendDataInterval) {
              lastSendTime = currentMillis2;

              // Điều chỉnh giá trị SpO2 và HR để nằm trong khoảng hợp lệ
              float adjustedSpo2 = (medianSpo2 > 100) ? 100 : medianSpo2;
              float adjustedHR = (avgHeartRate > 100) ? 100 : avgHeartRate;

              Serial.print("<SpO2:");
              Serial.print(adjustedSpo2);
              Serial.print(",HR:");
              Serial.print(adjustedHR);
              Serial.println(">");
            }
          }
        }
      }
    }
  }
}

void onBeatDetected() {
  // callback if 
}

float calculateMedian(float arr[], int size) {
  float sorted[size];
  memcpy(sorted, arr, size * sizeof(float));
  sortArray(sorted, size);
  if (size % 2 == 0) {
      return (sorted[size / 2 - 1] + sorted[size / 2]) / 2.0;
  } else {
      return sorted[size / 2];
  }
}

void sortArray(float arr[], int size) {
  for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        float temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
}

void resetBuffer() {
  discardCount = 0;
  spo2Index = 0;
  bufferFullspo2 = false;
  memset(spo2Values, 0, sizeof(spo2Values));
}

void resetHeartRateBuffer() {
  validReadingsCount = 0;
  sampleIndexHR = 0;
  bufferFilledHR = false;
  memset(heartRateSamples, 0, sizeof(heartRateSamples)); 
}