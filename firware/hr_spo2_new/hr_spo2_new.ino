#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

// Constants
#define IR_THRESHOLD 5000
#define REPORTING_PERIOD_MS 100   // Cập nhật dữ liệu mỗi 100ms
#define SEND_DATA_INTERVAL 2000   // Gửi dữ liệu mỗi 2 giây
#define BUFFER_SIZE_SPO2 21       // Kích thước bộ đệm SpO2
#define BUFFER_SIZE_HR 20         // Kích thước bộ đệm HR
#define DISCARD_LIMIT 10          // Số lần bỏ qua ban đầu để ổn định
#define SPO2_MIN 70               // Giá trị SpO2 tối thiểu hợp lệ
#define SPO2_MAX 100              // Giá trị SpO2 tối đa
#define HR_MIN 40                 // Giá trị HR tối thiểu hợp lệ
#define HR_MAX 120                // Giá trị HR tối đa

// Calibration factors (được điều chỉnh từ dữ liệu thực tế)
#define CALIBRATION_FACTOR_SPO2 1.0189
#define OFFSET_SPO2 0.2892
#define CALIBRATION_FACTOR_HR 1.0133
#define OFFSET_HR -0.0135

// Global variables
PulseOximeter pox;
uint32_t lastReportTime = 0;
uint32_t lastSendTime = 0;

// Buffers for SpO2 and HR
float spo2Buffer[BUFFER_SIZE_SPO2];
float hrBuffer[BUFFER_SIZE_HR];
uint8_t spo2Index = 0;
uint8_t hrIndex = 0;
bool spo2BufferFull = false;
bool hrBufferFull = false;
uint8_t discardCount = 0;
bool fingerDetected = true;

// Median and average values
float medianSpO2 = 0.0;
float avgHeartRate = 0.0;
float lastSentSpO2 = -1.0;
float lastSentHR = -1.0;

void setup() {
  Serial.begin(115200);

  // Khởi tạo cảm biến MAX30100 với số lần thử tối đa
  if (!initializeSensor()) {
    // Serial.println("Failed to initialize MAX30100 after multiple attempts.");
    while (true); // Dừng chương trình nếu khởi tạo thất bại
  }

  // Gán hàm callback khi phát hiện nhịp tim
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
  uint32_t currentTime = millis();
  pox.update(); // Cập nhật dữ liệu từ cảm biến

  // Cập nhật dữ liệu mỗi REPORTING_PERIOD_MS
  if (currentTime - lastReportTime >= REPORTING_PERIOD_MS) {
    processSensorData();
    lastReportTime = currentTime;
  }

  // Gửi dữ liệu mỗi SEND_DATA_INTERVAL
  if (currentTime - lastSendTime >= SEND_DATA_INTERVAL && spo2BufferFull && hrBufferFull) {
    sendData();
    lastSendTime = currentTime;
  }
}

// Khởi tạo cảm biến với retry
bool initializeSensor() {
  const int maxAttempts = 5;
  for (int attempt = 0; attempt < maxAttempts; attempt++) {
    if (pox.begin()) {
      // Serial.println("MAX30100 initialized successfully");
      return true;
    }
    delay(1000); // Đợi 1 giây trước khi thử lại
  }
  return false;
}

// Callback khi phát hiện nhịp tim
void onBeatDetected() {
  // Có thể thêm logic (ví dụ: nhấp nháy LED) nếu cần
}

// Xử lý dữ liệu từ cảm biến
void processSensorData() {
  float rawSpO2 = pox.getSpO2();
  float rawHR = pox.getHeartRate();

  // Kiểm tra xem có ngón tay hay không dựa trên giá trị thô
  if (rawSpO2 <= 0 && rawHR <= 0) {
    if (fingerDetected) {  // Nếu trước đó có ngón tay
      Serial.println("NOHAND");
      fingerDetected = false;
    }
    resetBuffers();
    return;
  } else {
    fingerDetected = true;  // Có ngón tay, tiếp tục xử lý
  }

  // Áp dụng hiệu chỉnh
  float adjustedSpO2 = constrain(rawSpO2 * CALIBRATION_FACTOR_SPO2 + OFFSET_SPO2, SPO2_MIN, SPO2_MAX);
  float adjustedHR = constrain(rawHR * CALIBRATION_FACTOR_HR + OFFSET_HR, HR_MIN, HR_MAX);

  // Lọc và lưu trữ SpO2
  if (adjustedSpO2 >= SPO2_MIN && adjustedSpO2 <= SPO2_MAX) {
    if (discardCount < DISCARD_LIMIT) discardCount++;
    else {
      spo2Buffer[spo2Index] = adjustedSpO2;
      spo2Index = (spo2Index + 1) % BUFFER_SIZE_SPO2;
      if (spo2Index == 0) spo2BufferFull = true;
      if (spo2BufferFull) medianSpO2 = calculateMedian(spo2Buffer, BUFFER_SIZE_SPO2);
    }
  }

  // Lọc và lưu trữ HR
  if (adjustedHR >= HR_MIN && adjustedHR <= HR_MAX) {
    hrBuffer[hrIndex] = adjustedHR;
    hrIndex = (hrIndex + 1) % BUFFER_SIZE_HR;
    if (hrIndex == 0) hrBufferFull = true;
    if (hrBufferFull) avgHeartRate = calculateAverage(hrBuffer, BUFFER_SIZE_HR);
  }
}

// Tính trung vị cho SpO2
float calculateMedian(float arr[], int size) {
  float temp[size];
  memcpy(temp, arr, size * sizeof(float));
  sortArray(temp, size);
  return (size % 2 == 0) ? (temp[size / 2 - 1] + temp[size / 2]) / 2.0 : temp[size / 2];
}

// Tính trung bình cho HR
float calculateAverage(float arr[], int size) {
  float sum = 0.0;
  for (int i = 0; i < size; i++)
    sum += arr[i];
  return sum / size;
}

// Sắp xếp mảng (bubble sort)
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

// Đặt lại bộ đệm
void resetBuffers() {
  discardCount = 0;
  spo2Index = 0;
  hrIndex = 0;
  spo2BufferFull = false;
  hrBufferFull = false;
  memset(spo2Buffer, 0, sizeof(spo2Buffer));
  memset(hrBuffer, 0, sizeof(hrBuffer));
  medianSpO2 = 0.0;
  avgHeartRate = 0.0;
}

// Gửi dữ liệu qua Serial
void sendData() {
  if (medianSpO2 > 0 && avgHeartRate > 0) {
    if (lastSentSpO2 != medianSpO2 || lastSentHR != avgHeartRate) {
      Serial.print("<SpO2:");
      Serial.print(medianSpO2, 1);
      Serial.print(",HR:");
      Serial.print(avgHeartRate, 0);
      Serial.println(">");
      lastSentSpO2 = medianSpO2;
      lastSentHR = avgHeartRate;
    }
  } 
}