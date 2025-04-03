#ifndef CONFIG_H
#define CONFIG_H

struct Data
{
    float temp;
    int spo2;
    int hr;
    int sys;
    int dia;
};

enum Wifi_Status
{
    DISCONNECTED = 0,
    CONNECTED,
    IDLE
};

#define BUZZER_PIN      12 // Chân điều khiển buzzer
#define SILENT_PIN      13 
#define LED_DATA_PIN    10
#define LED_ERR_PIN     11

const uint32_t DEBOUNCE_DELAY = 50;

bool is_remote = false;
bool is_silent = false;
bool is_nohand = true;
uint8_t result; 
uint8_t button_state;

#endif