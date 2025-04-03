#ifndef TFT_SCREEN_H
#define TFT_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <SdFat.h>

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   480

// Định nghĩa các chân SPI cho thẻ nhớ
#define SD_CS   53  // Chân SS cho thẻ nhớ
#define SD_SCK  52
#define SD_D0   50
#define SD_D1   51

class TFT_Screen {
public:
    TFT_Screen();
    void init();
    void displayFromSD(const char *filename);
    void displayInformation();
    void disableSDCard();
    void clearScreen();
    void drawWifiIcon(int x, int y, int radius, uint16_t color);
    void drawBatteryIcon(int x, int y);
    void drawTime(uint8_t hour, uint8_t minute);
    void drawSlashes(int x, int y, int radius, uint16_t color);
    void drawHorizontalLine();
    void updateTempValue(float temp);
    void updateSpO2Value(int spo2);
    void updateHRValue(int hr);
    void updatePressureValue(int sys, int dia);
    void displayLastData(float temp, int spo2, int hr, int sys, int dia);
    void displayChatbotMsg(String msg);

    

private:
    MCUFRIEND_kbv tft;
    SdFat SD;

    void drawLastTemp(float temp);
    void drawLastSpO2(int spo2);
    void drawLastHR(int hr);
    void drawLastPressure(int sys, int dia);
    
    void drawBMP(FsFile &file);
    void displayTextInQuadrants();
    void drawPartialCircle(int x, int y, int r, int start_angle, int end_angle, uint16_t color);
};

#endif