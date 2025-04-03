#include <Wire.h>
#include <OneWire.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include <RTClib.h>
#include <SPI.h>
#include <SdFat.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <ArduinoJson.h>

String previousSpO2Value = "";
String previousHRValue = "";
String previousSysValue = "";
String previousDiaValue = "";
String previousTempValue = "";

String spo2 = "";
String heartRate = "";

String tempValueFinal = "---" ;  // Round to 1 decimal place
String spo2ValueFinal= "---" ;       // Convert to integer
String heartRateValueFinal = "---" ;
String sysValueFinal = "---";
String diaValueFinal = "---";

String chatbotMsg;

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 50; // 1 second interval
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 50; // 50 seconds interval

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Định nghĩa kích thước màn hình
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

SdFat SD;
// Định nghĩa các chân SPI cho thẻ nhớ
#define SD_CS 53  // Chân SS cho thẻ nhớ
#define SD_SCK 52
#define SD_D0 50
#define SD_D1 51

MCUFRIEND_kbv tft;

RTC_DS3231 rtc;
DateTime t;
unsigned long lastTimeUpdate = 0; // Thời gian cập nhật thời gian cuối cùng
unsigned long previousMillisLED = 0; // Biến để lưu trữ thời gian lần cập nhật cuối
const long intervalLED = 500;        // Khoảng thời gian (500ms)

bool ledState = LOW;
#define BUZZER_PIN 12 // Chân điều khiển buzzer
#define BUTTON_PIN_Silient 13 // Chân điều khiển nút nhấn
bool isSilenced = false; // Trạng thái để kiểm tra xem lỗi đã được tắt bởi nút nhấn hay chưa
unsigned long lastButtonPressSilent = 0;  // Biến để lưu trữ thời gian lần nhấn nút cuối
const unsigned long debounceDelaySilent = 50; // Thời gian chống dội là 50ms


#define DATA_LED_PIN 10 // Chân đèn LED khi nhận dữ liệu

String lastCommand = "NO";
bool isRemoteRunning = false;

void setup() {
 
  // Initialize serial communication for ATmega328P connections
  Serial.begin(115200);      // HUYETAP
  Serial1.begin(9600);     // Serial1 - ATmega328P 1
  Serial2.begin(115200);     // Serial2 - ATmega328P 2
  Serial3.begin(9600);     // ESP

  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);
  pinMode(BUZZER_PIN, OUTPUT); // Khai báo chân BUZZER_PIN là OUTPUT
  digitalWrite(BUTTON_PIN_Silient, LOW); // Tắt buzzer ban đầu
  pinMode(DATA_LED_PIN, OUTPUT); // Khai báo chân LED là OUTPUT
  digitalWrite(DATA_LED_PIN, LOW); // Tắt LED ban đầu

  // Đọc ID màn hình và khởi tạo màn hình
  uint16_t ID = tft.readID();
  tft.begin(ID);

  // Cài đặt màn hình
  tft.setRotation(2);
  tft.fillScreen(BLACK); 
  //Khởi tạo RTC DS3231
  rtc.begin();
    // Khởi tạo thẻ nhớ
  if (!SD.begin(SD_CS)) {
      Serial.println("Không thể khởi tạo thẻ nhớ.");
      return;
  }
  Serial.println("Thẻ nhớ đã khởi tạo.");

  // Hiển thị hình ảnh từ thẻ nhớ
  displayImageFromSD("logo.bmp"); 

  // Hiển thị nội dung khác sau logo
  displayContent();

  // Dừng lại 2 giây trước khi chuyển tiếp
  delay(2000);

  // Vô hiệu hóa thẻ SD để tránh xung đột giao tiếp
  SD.end();
  digitalWrite(SD_CS, HIGH);  // Đặt chân CS của thẻ SD lên mức HIGH để vô hiệu hóa
  pinMode(MOSI, INPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, INPUT);

  // Xóa màn hình để chuẩn bị cho phần tiếp theo
  tft.fillScreen(BLACK);

  //Serial3.println("WIFI"); // Gửi dữ liệu để esp kết nối wifi

  // Chia màn hình thành 5 đường ngang
  drawHorizontalLines();

  // Biểu tượng cục pin
  drawBatteryIcon(280, 10);
  // Biểu tượng WiFi kế bên cục pin
  //drawWiFiIcon(260, 20, 15, WHITE);

  //Cập nhật thời gian lần đầu
  t = rtc.now();
  updateDisplayRealtime();
}

void loop() {
  if (Serial3.available() > 0) {
    // Đọc dữ liệu từ Serial3
    // String input = Serial3.readStringUntil('\n');
    while (Serial3.available() > 0) {
      char c = Serial3.read();
      input += c;
      if (c == '\n') break;
      delay(1);
    }
    input.trim();  // Loại bỏ ký tự xuống dòng và khoảng trắng ở đầu/cuối
    Serial.println("Serial 3: " + input);
    if(input.startsWith("{\"topic\":")){
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, input);

      if (!error) {
        const char* topic = doc["topic"];
        if (strcmp(topic, "chatbot") == 0) { // strcmp trả về 0 nếu chuỗi bằng nhau
          chatbotMsg = doc["msg"].as<String>(); // Lấy msg và gán vào chatbotMsg
        }
        Serial.println(chatbotMsg);
        // displayChatbotMsg(chatbotMsg);
        // delay(2000);
      }
    }
    // Kiểm tra xem dữ liệu nhận từ ESP là gì
    if (input == "OKWIFI") {
        tft.fillRect(250, 0, 30, 30, BLACK); 
        drawWiFiIcon(260, 20, 15, WHITE);  // Vẽ Wi-Fi không có dấu gạch chéo
    } else if (input == "NOWIFI") {
        tft.fillRect(250, 0, 30, 30, BLACK); 
        drawWiFiIconWithSlash(260, 20, 15, WHITE);  // Vẽ Wi-Fi có dấu gạch chéo
    } else if (input == "START") {
        // Xử lý START
        isRemoteRunning = true;
        Serial.println("START_ATMEGA328P");  // Gửi lệnh START đến ATMEGA328P
    } else if (input == "STOP") {
        // Xử lý STOP
        isRemoteRunning = false;
        Serial.println("STOP_ATMEGA328P");
    } else if (input == "SILENT") {
        // Xử lý SILENT
        isSilenced = true;           
        digitalWrite(11, LOW);
        noTone(BUZZER_PIN);
    }
  }
  unsigned long currentMillis1 = millis();
  unsigned long currentMillisLED = millis();
  unsigned long currentMillisSilient = millis();

  if (currentMillis1 - lastTimeUpdate >= 1000) {
    t = rtc.now();
    updateDisplayRealtime();
    lastTimeUpdate = currentMillis1;
  }
  unsigned long currentTime = millis();
  if (Serial1.available()) {
    digitalWrite(DATA_LED_PIN, HIGH); // Bật LED khi nhận dữ liệu
    String tempData = Serial1.readStringUntil('>');
    // Serial.println("Serial1: " + tempData);
    if (tempData.startsWith("<TEMP:")) {
      String tempValue = tempData.substring(6);
      if (tempValue != previousTempValue) {
        previousTempValue = tempValue;
      }
    }
  }

  if (Serial2.available()) {
    digitalWrite(DATA_LED_PIN, HIGH); // Bật LED khi nhận dữ liệu
    String spo2Data = Serial2.readStringUntil('>');
    spo2Data.trim();  
    // Serial.println("Serial2: " + spo2Data);
    if (spo2Data.startsWith("<SpO2:")) {
      spo2 = spo2Data.substring(6, spo2Data.indexOf(","));
      heartRate = spo2Data.substring(spo2Data.indexOf("HR:") + 3);
      if (spo2 != previousSpO2Value) {
        previousSpO2Value = spo2;
      }
      if (heartRate != previousHRValue) {
        previousHRValue = heartRate;
      }
    }
  }
  // Check Serial for pressure data
  if (Serial.available()) {
    digitalWrite(DATA_LED_PIN, HIGH); // Bật LED khi nhận dữ liệu
    String pressureData = Serial.readStringUntil('>');
    Serial.println("Serial: " + pressureData);
    if (pressureData.startsWith("<SYS:") || pressureData.indexOf(",") != -1 && pressureData.indexOf("DIA:") != -1) {
      String sysValue = pressureData.substring(pressureData.indexOf("SYS:") + 4, pressureData.indexOf(","));
      String diaValue = pressureData.substring(pressureData.indexOf("DIA:") + 4);
      Serial.println("Sys Value: " + sysValue);
      Serial.println("Dia Value: " + diaValue);
      if (sysValue != previousSysValue) {
          previousSysValue = sysValue;
      }
      if (diaValue != previousDiaValue) {
          previousDiaValue = diaValue;           
        }
     }
  }
  tempValueFinal = previousTempValue == "" ? "---" : String(previousTempValue.toFloat(), 1);
  spo2ValueFinal = previousSpO2Value == "" ? "---" : String(previousSpO2Value.toInt());
  // heartRateValueFinal = previousHRValue == "" ? "---" : String(previousHRValue.toInt());
  heartRateValueFinal = (previousHRValue == "" || previousHRValue == "0" ) ? "---" : String(previousHRValue.toInt());
  if (heartRateValueFinal==0) {heartRateValueFinal=80;} 
  sysValueFinal = previousSysValue == "" ? "--" : String(previousSysValue.toInt());
  diaValueFinal = previousDiaValue == "" ? "--" : String(previousDiaValue.toInt());

// Kiểm tra điều kiện nếu có lỗi (giống như đoạn mã ban đầu)
  bool hasError = (tempValueFinal != "---" && tempValueFinal.toFloat() > 38) ||
  (spo2ValueFinal != "---" && spo2ValueFinal.toInt() < 94) ||
  (heartRateValueFinal != "---" && 
  (heartRateValueFinal.toInt() < 60))|| //heartRateValueFinal.toInt() > 100)) ||
  (sysValueFinal != "--" && diaValueFinal != "--" && 
  (sysValueFinal.toInt() < 90 || diaValueFinal.toInt() < 60 || sysValueFinal.toInt() > 130 || diaValueFinal.toInt() > 80));

  // Chống dội cho nút nhấn
  if (digitalRead(BUTTON_PIN_Silient) == HIGH) {
    if (currentMillisSilient - lastButtonPressSilent >= debounceDelaySilent) {
      if (digitalRead(BUTTON_PIN_Silient) == HIGH) {
        lastButtonPressSilent = currentMillisSilient;
        // Nếu nút được nhấn, tắt LED và buzzer, và đặt cờ "isSilenced" là true
        isSilenced = true;
        digitalWrite(11, LOW);
        noTone(BUZZER_PIN);
      }
    }
  }

  if (hasError && !isSilenced) {
    // Nếu có lỗi, chớp tắt LED
    if (currentMillisLED - previousMillisLED >= intervalLED) {
      previousMillisLED = currentMillisLED;
      ledState = !ledState;
      digitalWrite(11, ledState ? HIGH : LOW);
    }
    //digitalWrite(BUZZER_PIN, HIGH); // Bật buzzer khi có lỗi
    tone(BUZZER_PIN, 1000); // Phát ra âm thanh với tần số 1000 Hz khi có lỗi
  } else {
    // Nếu không có lỗi, tắt LED
    digitalWrite(11, LOW);
    //digitalWrite(BUZZER_PIN, LOW); // Bật buzzer khi có lỗi
    noTone(BUZZER_PIN); // Dừng âm thanh của buzzer
  }
  // Update LCD every 1 second
  if (currentTime - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentTime;
    tft.fillRect(80, 70, 250, 60, BLACK);
    if (sysValueFinal == "--") {
    tft.setCursor(130, 80); // Display '--' at position 140 if no value
    } else //{
    //tft.setCursor(90, 80); // Display actual value at position 90 if available
    //}
    if (sysValueFinal.length() == 3) {
      tft.setCursor(90, 80); // Display 3-character value at position (90, 80)
    } else {
      tft.setCursor(130, 80); // Display other value at position (150, 80)
    }
    tft.setTextColor(YELLOW);  
    tft.setTextSize(6);
    tft.print(sysValueFinal);

    tft.setCursor(200, 80);
    tft.setTextColor(YELLOW);  
    tft.setTextSize(6);
    tft.print("/");

    tft.setCursor(240, 80);
    tft.setTextColor(YELLOW);  
    tft.setTextSize(6);
    tft.print(diaValueFinal);

    tft.fillRect(50, 190, 220, 50, BLACK);
    tft.setCursor(160, 190);
    tft.setTextColor(CYAN);  
    tft.setTextSize(6);
    tft.print(heartRateValueFinal);

    tft.fillRect(90, 300, 180, 50, BLACK);
    tft.setCursor(160, 300);
    tft.setTextColor(GREEN);  
    tft.setTextSize(6);
    tft.print(spo2ValueFinal);

    tft.fillRect(130, 400, 160, 70, BLACK);
    tft.setCursor(130, 420);
    tft.setTextColor(BLUE);
    tft.setTextSize(6);
    tft.print(tempValueFinal);

    delay(500);
  }

  // Send data to ESP8266 every 50 seconds
  if (currentTime - lastSendTime >= sendInterval) {
    lastSendTime = currentTime;
   // String dataToSend = "<TEMP:" + previousTempValue + ",SpO2:" + previousSpO2Value + ",HR:" + previousHRValue + ",SYS:" + previousSysValue + ",DIA:" + previousDiaValue + ">";
     String dataToSend = "<TEMP:" + tempValueFinal + ",SpO2:" + spo2ValueFinal + ",HR:" + heartRateValueFinal + ",SYS:" + sysValueFinal + ",DIA:" + diaValueFinal + ">";
    Serial.println("Sending to ESP8266: " + dataToSend);
    Serial3.print(dataToSend);  // Send data to ESP8266 via Serial
  }
}
void displayContent() {
  // Hiển thị nội dung văn bản
  tft.setFont(NULL);  // Font mặc định
  tft.setCursor(30, 170); // Cập nhật vị trí y để hiển thị văn bản dưới logo
  tft.setTextSize(2);
  tft.print("Nganh: Ky Thuat Y Sinh");

  // Hiển thị tiêu đề
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(0xFFFF, 0x0000);  // Màu chữ trắng với nền đen
  tft.setTextSize(1);
  tft.setCursor(40, 220); // Điều chỉnh vị trí y cho tiêu đề
  tft.print("DO AN TOT NGHIEP");

  // Hiển thị nội dung khác
  tft.setFont(&FreeSansBold12pt7b);
  tft.setCursor(50, 270); // Điều chỉnh vị trí y
  tft.println("MAY KIEM TRA VA ");
  tft.setCursor(30, 300); // Điều chỉnh vị trí y
  tft.print("THEO DOI SUC KHOE");

  // Hiển thị giáo viên và sinh viên
  tft.setFont(NULL);
  tft.setCursor(30, 330); // Điều chỉnh vị trí y
  tft.setTextSize(2);
  tft.print("GVHD: TS. Vu Chi Cuong");

  tft.setCursor(30, 360); // Điều chỉnh vị trí y
  tft.print("SVTH: CAO TIEN SY");

  tft.setCursor(90, 390); // Điều chỉnh vị trí y
  tft.print(" MAI DONG THUC");
}
void bmpDraw(FsFile &file) {
    if (file.read() != 'B' || file.read() != 'M') {
        Serial.println("Không phải file BMP.");
        return;
    }

    file.seek(18);
    int32_t width = file.read() + (file.read() << 8) + (file.read() << 16) + (file.read() << 24);
    int32_t height = file.read() + (file.read() << 8) + (file.read() << 16) + (file.read() << 24);
    file.seek(54);

    Serial.print("Kích thước hình ảnh: ");
    Serial.print(width);
    Serial.print("x");
    Serial.println(height);

    // Tính toán padding cho mỗi hàng
    int padding = (4 - (width * 3) % 4) % 4;

    // Tính toán vị trí bắt đầu để vẽ logo ở giữa theo chiều ngang
    int startX = (tft.width() - width) / 2;
    
    // Giữ logo ở một khoảng cố định từ phía trên (ví dụ 30 pixels)
    int startY = 10; // Thay đổi giá trị này để di chuyển logo lên hoặc xuống

    // Vẽ hình ảnh pixel từng dòng
    for (int32_t y = height - 1; y >= 0; y--) {
        for (int32_t x = 0; x < width; x++) {
            uint8_t b = file.read(); // Đọc màu xanh dương
            uint8_t g = file.read(); // Đọc màu xanh lá
            uint8_t r = file.read(); // Đọc màu đỏ
            tft.drawPixel(startX + x, startY + y, tft.color565(r, g, b)); // Vẽ pixel
        }
        // Bỏ qua padding nếu có
        for (int p = 0; p < padding; p++) {
            file.read();
        }
    }
}

void displayImageFromSD(const char* filename) {
    Serial.print("Mở file: ");
    Serial.println(filename);
    
    FsFile file = SD.open(filename, O_RDONLY);
    if (!file) {
        Serial.println("Không thể mở file.");
        return;
    }
    Serial.println("File đã mở.");

    // Hiển thị hình ảnh BMP từ thẻ nhớ lên màn hình TFT
    bmpDraw(file);

    file.close();
}
void updateDisplayRealtime()
{
  // Xóa khu vực hiển thị trước đó
    tft.fillRect(10, 10, 160, 20, BLACK); 
    // Hiển thị thời gian HH:MM:SS
    tft.setTextColor(WHITE);  
    tft.setTextSize(2);
    tft.setCursor(10, 10); 
    if (t.hour() < 10) tft.print('0');
    tft.print(t.hour());
    tft.print(":");
    if (t.minute() < 10) tft.print('0');
    tft.print(t.minute());
}

void displayChatbotMsg(const String msg) {
  tft.fillRect(0, 40, SCREEN_WIDTH, 20, BLACK); // Xóa vùng dưới thời gian/WiFi/pin
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 45);

  int maxCharsPerLine = SCREEN_WIDTH / 6; // Ước lượng mỗi ký tự chiếm 6px
  if (msg.length() > maxCharsPerLine) {
    tft.print(msg.substring(0, maxCharsPerLine)); // In tối đa 1 dòng
  } else {
    tft.print(msg);
  }
}

// Hàm vẽ biểu tượng cục pin
void drawBatteryIcon(int x, int y) {
    int width = 30;
    int height = 15;
    int terminalWidth = 4;
    int terminalHeight = 6;
    
    // Vẽ thân pin
    tft.drawRect(x, y, width, height, GREEN);
    
    // Vẽ phần đầu pin
    tft.fillRect(x + width, y + (height - terminalHeight) / 2, terminalWidth, terminalHeight, GREEN);
    
    // Vẽ mức pin (giả sử đầy 80%)
    int batteryLevel = 1 * (width - 4);
    tft.fillRect(x + 2, y + 2, batteryLevel, height - 4, GREEN);
}
// Hàm hiển thị văn bản ở mỗi phần
void displayTextInQuadrants() {
    // Đặt kích thước văn bản
    tft.setTextSize(6);
    tft.setTextColor(YELLOW);
    tft.setCursor(150, 90);
    tft.println("---");
    tft.setTextColor(CYAN);
    tft.setCursor(150, 200);
    tft.println("---");
    tft.setTextColor(GREEN);
    tft.setCursor(150, 310);
    tft.println("---");
    tft.setTextColor(BLUE);
    tft.setCursor(150, 430);
    tft.println("---");
    tft.setTextSize(2);

    // Văn bản ở phần 1
    tft.setTextColor(YELLOW); 
    tft.setCursor(100, 40); 
    tft.print("SYS/DIA       mmHg");
    tft.setCursor(10, 60);
    tft.println("119/79");
    tft.setCursor(10, 115);
    tft.println("90/60");

    // Văn bản ở phần 2
    tft.setTextColor(CYAN); 
    tft.setCursor(90, 152.5); // Phần trên bên trái 
    tft.print("HR             bpm");
    tft.setCursor(10, 172.5);
    tft.println("100");
    tft.setCursor(10, 225);
    tft.println("60");

    // Văn bản ở phần 3
    tft.setTextColor(GREEN);  
    tft.setCursor(90, 269.5); // Phần trên bên trái
    tft.print("SpO2             %");
    tft.setCursor(10, 285);
    tft.println("100");
    tft.setCursor(10, 335);
    tft.println("95");

    // Văn bản ở phần 4
    tft.setTextColor(BLUE); 
    tft.setCursor(90, 382); // Phần trên bên trái 
    tft.print("TEMP");
    tft.setCursor(10, 397.5);
    tft.println("37.2");
    tft.setCursor(10, 457);
    tft.println("36.1");
    tft.setCursor(290, 382);
    tft.setTextSize(2);
    tft.print((char)247);
    tft.print("C");
}
// Hàm vẽ các đường ngang chia màn hình
void drawHorizontalLines() {
    int firstLineHeight = 30; // Chiều cao của đường đầu tiên (khoảng cách nhỏ)
    int remainingHeight = SCREEN_HEIGHT - firstLineHeight; // Chiều cao còn lại
    int remainingLinesHeight = remainingHeight/4; // Chiều cao của 4 đường còn lại

    // Vẽ đường ngang đầu tiên
    tft.drawLine(0, 30, 320, 30, WHITE);

    // Vẽ 4 đường ngang còn lại
    for (int i = 1; i <= 3; i++) {
        int y = firstLineHeight + i * remainingLinesHeight;
        tft.drawLine(0, y, SCREEN_HEIGHT, y, WHITE);
    }
    // Hiển thị văn bản ở mỗi phần
    displayTextInQuadrants();
}

// Hàm vẽ biểu tượng WiFi có dấu gạch chéo (biểu tượng không bắt được WiFi)
void drawWiFiIconWithSlash(int x, int y, int radius, uint16_t color) {
    // Vẽ các cung tròn theo chiều dọc (90 độ quay về phía trên)
    drawPartialCircle(x, y, radius, 45, 135, color);       // Vòng lớn nhất
    drawPartialCircle(x, y, radius - 5, 45, 135, color);   // Vòng giữa
    drawPartialCircle(x, y, radius - 10, 45, 135, color);  // Vòng nhỏ nhất
    
    // Vẽ chấm tròn ở trung tâm
    tft.fillCircle(x, y, 1, color);

    // Vẽ dấu gạch chéo (đường chéo từ góc trên bên trái đến góc dưới bên phải)
    drawSlash(x , y - 10, radius, color);  // Vẽ dấu gạch chéo
}

// Hàm vẽ cung tròn (partial circle)
void drawPartialCircle(int x, int y, int r, int startAngle, int endAngle, uint16_t color) {
    for (int angle = startAngle; angle <= endAngle; angle++) {
        float radian = angle * 3.14159 / 180; // Chuyển đổi từ độ sang radian
        int x0 = x + r * cos(radian);         // Tọa độ X
        int y0 = y - r * sin(radian);         // Tọa độ Y
        tft.drawPixel(x0, y0, color);         // Vẽ pixel tại vị trí
    }
}

// Hàm vẽ dấu gạch chéo ngắn, đi qua trung tâm và cắt qua các cung tròn
void drawSlash(int x, int y, int radius, uint16_t color) {
    int lineLength = radius - 5;  // Độ dài của dấu gạch chéo, bạn có thể điều chỉnh độ dài ở đây
    
    // Vẽ đường chéo từ vị trí trên trái đến vị trí dưới phải (ngắn)
    int x1 = x - lineLength;  // Tọa độ bắt đầu trên trục X (từ bên trái)
    int y1 = y - lineLength;  // Tọa độ bắt đầu trên trục Y (từ trên)
    int x2 = x + lineLength;  // Tọa độ kết thúc trên trục X (từ bên phải)
    int y2 = y + lineLength ;  // Tọa độ kết thúc trên trục Y (từ dưới)

    // Vẽ đường chéo giữa (x1, y1) và (x2, y2)
    tft.drawLine(x1, y1 , x2 , y2 , color);
}
// Hàm vẽ biểu tượng WiFi không có dấu gạch chéo
void drawWiFiIcon(int x, int y, int radius, uint16_t color) {
    // Vẽ các cung tròn theo chiều dọc (90 độ quay về phía trên)
    drawPartialCircle(x, y, radius, 45, 135, color);       // Vòng lớn nhất
    drawPartialCircle(x, y, radius - 5, 45, 135, color);   // Vòng giữa
    drawPartialCircle(x, y, radius - 10, 45, 135, color);  // Vòng nhỏ nhất
    
    // Vẽ chấm tròn ở trung tâm
    tft.fillCircle(x, y, 1, color);
}