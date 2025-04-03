#include "TFT_Screen.h"

TFT_Screen::TFT_Screen() { 
}

void TFT_Screen::init() {
    uint16_t ID = tft.readID();
    tft.begin(ID);
    tft.setRotation(2);
    tft.fillScreen(BLACK);

    if (!SD.begin(SD_CS)) {
        Serial.println("SD card initialization failed!");
        return;
    }
}

void TFT_Screen::displayFromSD(const char *filename) {
    Serial.println("Opening file: " + String(filename));

    FsFile file = SD.open(filename, O_RDONLY);
    if (!file) {
        Serial.println("Failed to open file");
        return;
    }
    drawBMP(file);
    file.close();
}

void TFT_Screen::clearScreen() {
    tft.fillScreen(BLACK);
}

void TFT_Screen::disableSDCard() {
    SD.end();
    digitalWrite(SD_CS, HIGH);  // Đặt chân CS của thẻ SD lên mức HIGH để vô hiệu hóa
    pinMode(MOSI, INPUT);
    pinMode(MISO, INPUT);
    pinMode(SCK, INPUT);
}

void TFT_Screen::drawBMP(FsFile &file) {
    if (file.read() != 'B' || file.read() != 'M') {
        Serial.println("Not a valid BMP file!");
        return;
    }

    file.seek(18);
    int32_t width = file.read() + (static_cast<int32_t>(file.read()) << 8) + (static_cast<int32_t>(file.read()) << 16) + (static_cast<int32_t>(file.read()) << 24);
    int32_t height = file.read() + (static_cast<int32_t>(file.read()) << 8) + (static_cast<int32_t>(file.read()) << 16) + (static_cast<int32_t>(file.read()) << 24);
    file.seek(54);

    // Serial.print("Image size: ");
    // Serial.print(width);
    // Serial.print("x");
    // Serial.println(height);

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

void TFT_Screen::displayInformation() {
    // Hiển thị nội dung văn bản
    tft.setFont(NULL);  // Font mặc định
    tft.setCursor(125, 170);
    tft.setTextSize(2);
    tft.print("NGANH:");

    tft.setCursor(40, 200); 
    tft.setTextSize(2);
    tft.print("He thong Nhung & IoT");

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(0xFFFF, 0x0000);  
    tft.setTextSize(1);
    tft.setCursor(40, 260); 
    tft.print("DO AN TOT NGHIEP");

    tft.setFont(&FreeSansBold12pt7b);
    tft.setCursor(50, 310); 
    tft.println("MAY KIEM TRA VA ");
    tft.setCursor(30, 340); 
    tft.print("THEO DOI SUC KHOE");

    tft.setFont(NULL);
    tft.setCursor(30, 370); 
    tft.setTextSize(2);
    tft.print("GVHD: TS. Vu Chi Cuong");
    tft.setCursor(30, 400); 
    tft.print("SVTH: CAO TIEN SY");
    tft.setCursor(90, 430); 
    tft.print(" MAI DONG THUC");
} 

void TFT_Screen::drawWifiIcon(int x, int y, int radius, uint16_t color) {
    tft.fillRect(250, 0, 30, 30, BLACK);

    drawPartialCircle(x, y, radius, 45, 135, color);       // Vòng lớn nhất
    drawPartialCircle(x, y, radius - 5, 45, 135, color);   // Vòng giữa
    drawPartialCircle(x, y, radius - 10, 45, 135, color);  // Vòng nhỏ nhất
    
    tft.fillCircle(x, y, 1, color);
}

void TFT_Screen::drawBatteryIcon(int x, int y) {
    // Serial.println("Drawing battery icon");
    static int width = 30;
    static int height = 15;
    static int terminalWidth = 4;
    static int terminalHeight = 6;
    
    tft.drawRect(x, y, width, height, GREEN);
    tft.fillRect(x + width, y + (height - terminalHeight) / 2, terminalWidth, terminalHeight, GREEN);
    
    // Vẽ mức pin (giả sử đầy 80%)
    static int batteryLevel = 1 * (width - 4);
    tft.fillRect(x + 2, y + 2, batteryLevel, height - 4, GREEN);
}   

void TFT_Screen::drawTime(uint8_t hour, uint8_t minute) {
    // Serial.println("Drawing time");
    tft.fillRect(10, 10, 160, 20, BLACK); 
    tft.setTextColor(WHITE);  
    tft.setTextSize(2);
    tft.setCursor(10, 10); 
    if (hour < 10) tft.print('0');
    tft.print(hour);
    tft.print(":");
    if (minute < 10) tft.print('0');
    tft.print(minute);
}

void TFT_Screen::drawSlashes(int x, int y, int radius, uint16_t color) {
    static int lineLength = radius - 5; 
    
    int x1 = x - lineLength;  
    int y1 = y - lineLength;  
    int x2 = x + lineLength;  
    int y2 = y + lineLength ;  
    tft.drawLine(x1, y1 , x2 , y2 , color);
}

void TFT_Screen::drawHorizontalLine() {
    // Serial.println("Drawing horizontal lines");
    static int firstLineHeight = 30; // Chiều cao của đường đầu tiên (khoảng cách nhỏ)
    static int remainingHeight = SCREEN_HEIGHT - firstLineHeight; // Chiều cao còn lại
    static int remainingLinesHeight = remainingHeight/4; // Chiều cao của 4 đường còn lại

    tft.drawLine(0, 30, 320, 30, WHITE);

    for (int i = 1; i <= 3; i++) {
        int y = firstLineHeight + i * remainingLinesHeight;
        tft.drawLine(0, y, SCREEN_HEIGHT, y, WHITE);
    }
    displayTextInQuadrants();
}

void TFT_Screen::drawPartialCircle(int x, int y, int r, int start_angle, int end_angle, uint16_t color) {
    for (int angle = start_angle; angle <= end_angle; angle++) {
        float radian = angle * 3.14159 / 180; // Chuyển đổi từ độ sang radian
        int x0 = x + r * cos(radian);         // Tọa độ X
        int y0 = y - r * sin(radian);         // Tọa độ Y
        tft.drawPixel(x0, y0, color);         // Vẽ pixel tại vị trí
    }
}

void TFT_Screen::displayTextInQuadrants() {
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
    tft.setCursor(90, 40); 
    tft.print("SYS|DIA        mmHg");
    tft.setCursor(10, 60);
    tft.println("119|79");
    tft.setCursor(10, 115);
    tft.println("90|60");

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

void TFT_Screen::updateTempValue(float temp) {
    // Serial.println("Updating temperature value");
    tft.fillRect(120, 420, 135, 70, BLACK);
    tft.setCursor(120, 420);
    tft.setTextColor(BLUE);
    tft.setTextSize(5);
    // tft.print(temp);
    if (temp != -159.7){
        tft.print((int)temp);
        tft.print(".");
        tft.print((int)(temp * 10) % 10);
    } else {
        tft.print("ERR");
    }
}

void TFT_Screen::updateSpO2Value(int spo2) {
    // Serial.println("Updating SpO2 value");
    tft.fillRect(150, 300, 110, 50, BLACK);
    tft.setCursor(150, 300);
    tft.setTextColor(GREEN);  
    tft.setTextSize(5);
    tft.print(spo2);
}

void TFT_Screen::updateHRValue(int hr) {
    // Serial.println("Updating HR value");
    tft.fillRect(150, 190, 110, 50, BLACK);
    tft.setCursor(150, 190);
    tft.setTextColor(CYAN);  
    tft.setTextSize(5);
    tft.print(hr);
}

void TFT_Screen::updatePressureValue(int sys, int dia) {
    // Serial.println("Updating pressure value");
    tft.fillRect(80, 80, 250, 60, BLACK);
    if (sys >= 100) {
        tft.setCursor(90, 80);;
    } else {
        tft.setCursor(110, 80);
    }
    tft.setTextColor(YELLOW);  
    tft.setTextSize(5);
    tft.print(sys);
    tft.print("|");
    tft.print(dia);   
}

void TFT_Screen::displayChatbotMsg(String msg) {
    // Serial.println("Displaying chatbot message");
    tft.fillRect(0, 0, 320, 30, BLACK);
    tft.setCursor(10, 10);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Chatbot: ");
    tft.print(msg);
}

void TFT_Screen::displayLastData(float temp, int spo2, int hr, int sys, int dia) {
    drawLastTemp(temp);
    drawLastSpO2(spo2);
    drawLastHR(hr);
    drawLastPressure(sys, dia);
}

void TFT_Screen::drawLastTemp(float temp) {
    tft.fillRect(265, 450, 50, 30, BLACK);
    tft.setCursor(265, 450);
    tft.setTextColor(BLUE);  
    tft.setTextSize(2);
    if (temp != -159.7){
        tft.print((int)temp);
        tft.print(".");
        tft.print((int)(temp * 10) % 10);
    }
}

void TFT_Screen::drawLastSpO2(int spo2) {
    tft.fillRect(270, 320, 40, 30, BLACK);
    tft.setCursor(270, 320);
    tft.setTextColor(GREEN);  
    tft.setTextSize(3);
    tft.print(spo2); 
}

void TFT_Screen::drawLastHR(int hr) {
    tft.fillRect(270, 210, 40, 30, BLACK);
    tft.setCursor(270, 210);
    tft.setTextColor(CYAN);  
    tft.setTextSize(3);
    tft.print(hr);
}

void TFT_Screen::drawLastPressure(int sys, int dia) {
    tft.fillRect(270, 80, 40, 50, BLACK);
    tft.setCursor(270, 80);
    tft.setTextColor(YELLOW);  
    tft.setTextSize(2);
    tft.print(sys);
    tft.setCursor(270, 105);
    tft.print("|");
    tft.print(dia);
}