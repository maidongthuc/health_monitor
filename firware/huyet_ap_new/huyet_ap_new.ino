#include <avr/interrupt.h>
#include <Wire.h>

// Định nghĩa trạng thái
#define ON HIGH
#define OFF LOW
enum MotorState { START_STATE, INFLATE2_STATE, DEFLATE_STATE, DISPLAY_STATE, RESET_STATE };
enum MeasState { SYS_MEASURE, SYS_CAL, RATE_MEASURE, DIAS_MEASURE, DIAS_CAL, STOP_STATE };

// Định nghĩa chân
#define DT_PIN 2
#define SCK_PIN 3
#define VALVE_PIN 4
#define MOTOR_PIN 8
#define BT_START 7

// Cấu trúc dữ liệu
typedef struct {
    float systolic, diastolic, pulse_per_min;
    bool bpMeasured;
} MeasurementData;

typedef struct {
    unsigned int timepress0, timing, timerate, timerun_dias, timecount, timedeflate, timedisplay;
} TimingData;

typedef struct {
    enum MotorState currentState;
    enum MeasState measState;
    unsigned char sysCount, countAverage, countPulse, count, stopCount;
} StateData;

typedef struct {
    float sys, rate, dias;
    float sys2, rate2, dias2;
} ThresholdData;

// Khai báo biến toàn cục
MeasurementData measurements = {0, 0, 0, false};
TimingData timers = {0};
StateData states = {START_STATE, SYS_MEASURE, 0, 0, 0, 0, 0};
ThresholdData thresholds = {4.2, 4.0, 3.5, 3.8, 3.5, 3.3};
float maxPressure = 215.0;
float former = 0;
float lastSentSystolic = -1.0;   // Giá trị ban đầu không hợp lệ để buộc gửi lần đầu
float lastSentDiastolic = -1.0;

// Biến điều khiển
bool isPhysicalRunning = false, isRemoteRunning = false;
bool buttonState = LOW, lastButtonState = LOW;
unsigned long lastDebounceTime = 0, debounceDelay = 50;
unsigned long lastBpSendTime = 0, bpSendInterval = 1000;
String lastCommand = "NO";

// Hàm khởi tạo
void setup() {
    Serial.begin(115200);
    pinMode(SCK_PIN, OUTPUT);
    pinMode(DT_PIN, INPUT);
    pinMode(BT_START, INPUT);
    pinMode(MOTOR_PIN, OUTPUT);
    pinMode(VALVE_PIN, OUTPUT);
    digitalWrite(SCK_PIN, LOW);
    digitalWrite(VALVE_PIN, OFF);
    digitalWrite(MOTOR_PIN, OFF);

    cli();
    TCCR1A = 0; TCCR1B = 0; TIMSK1 = 0;
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10); // Prescale 64, CTC mode
    OCR1A = 249; // 1ms interrupt
    TIMSK1 = (1 << OCIE1A);
    sei();

    former = thresholds.sys - 0.01;
    timers.timing = 40;
}

// Xử lý ngắt Timer1
ISR(TIMER1_COMPA_vect) {
    timers.timecount++;
    timers.timedeflate++;
    if (digitalRead(BT_START)) {
        timers.timepress0 = (timers.timepress0 < 0xFFFF) ? timers.timepress0 + 1 : 0xFFFF;
    } else {
        timers.timepress0 = 0;
    }
    if (timers.timing > 0) timers.timing--;
    if (timers.timerate < 6000) timers.timerate++;
    if (timers.timerun_dias < 2000) timers.timerun_dias++;
    if (timers.timedisplay < 2000) timers.timedisplay++;
}

// Đọc dữ liệu từ HX710B
long readHX710B() {
    long count = 0;
    digitalWrite(SCK_PIN, LOW);
    delayMicroseconds(1);
    while (digitalRead(DT_PIN)); // Chờ DT_PIN xuống LOW
    for (int i = 0; i < 24; i++) {
        digitalWrite(SCK_PIN, HIGH);
        delayMicroseconds(1);
        count = count << 1;
        digitalWrite(SCK_PIN, LOW);
        delayMicroseconds(1);
        if (digitalRead(DT_PIN)) count++;
    }
    digitalWrite(SCK_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(SCK_PIN, LOW);
    delayMicroseconds(1);
    if (count & 0x800000) count |= ~0xFFFFFF; // Xử lý số âm
    return count;
}

// Đọc áp suất từ HX710B
float readPressure(bool subtractOffset) {
    long data = readHX710B();
    float adc = (((float)data) / 8388607.0) * 5.0;
    if (subtractOffset) adc -= 0.41;
    return (adc / 128) / 0.05 * 300;
}

// Gửi dữ liệu huyết áp
void sendBloodPressureData() {
  if(lastSentSystolic != measurements.systolic || lastSentDiastolic != measurements.diastolic){
    Serial.print("<SYS:");
    Serial.print(measurements.systolic, 2);
    Serial.print(",DIA:");
    Serial.print(measurements.diastolic, 2);
    Serial.println(">");
    lastSentSystolic = measurements.systolic;
    lastSentDiastolic = measurements.diastolic;
  }
    
}

// Nhận lệnh từ ESP8266
void receiveCommandFromESP() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        if (command != lastCommand) {
            lastCommand = command;
            if (command == "START_ATMEGA328P" && !isRemoteRunning) toggleRemoteMotor();
            else if (command == "STOP_ATMEGA328P" && isRemoteRunning) toggleRemoteMotor();
        }
    }
}

// Chống dội nút nhấn
void debounceButton() {
    int reading = digitalRead(BT_START);
    if (reading != lastButtonState) lastDebounceTime = millis();
    if ((millis() - lastDebounceTime) > debounceDelay && reading != buttonState) {
        buttonState = reading;
        if (buttonState == HIGH) togglePhysicalMotor();
    }
    lastButtonState = reading;
}

// Điều khiển motor từ nút nhấn
void togglePhysicalMotor() {
    if (isPhysicalRunning) {
        isPhysicalRunning = false;
        states.currentState = RESET_STATE;
        digitalWrite(MOTOR_PIN, OFF);
        digitalWrite(VALVE_PIN, ON);
    } else {
        isPhysicalRunning = true;
        states.currentState = START_STATE;
        digitalWrite(VALVE_PIN, OFF);
        digitalWrite(MOTOR_PIN, ON);
    }
}

// Điều khiển motor từ ESP8266
void toggleRemoteMotor() {
    if (isRemoteRunning) {
        isRemoteRunning = false;
        states.currentState = RESET_STATE;
        digitalWrite(MOTOR_PIN, OFF);
        digitalWrite(VALVE_PIN, ON);
    } else {
        isRemoteRunning = true;
        states.currentState = INFLATE2_STATE;
        digitalWrite(VALVE_PIN, OFF);
        digitalWrite(MOTOR_PIN, ON);
    }
}

// Trạng thái START
void startState() {
    states.sysCount = 0;
    measurements.systolic = measurements.diastolic = measurements.pulse_per_min = 0;
    states.count = states.stopCount = states.countAverage = states.countPulse = 0;
    timers.timepress0 = timers.timerate = timers.timerun_dias = timers.timecount = 0;
    timers.timing = 40;
    former = thresholds.sys - 0.01;
    states.measState = SYS_MEASURE;

    if (digitalRead(BT_START) && timers.timepress0 > 30) {
        states.currentState = INFLATE2_STATE;
        timers.timepress0 = timers.timecount = 0;
        digitalWrite(VALVE_PIN, OFF);
        digitalWrite(MOTOR_PIN, ON);
    }
}

// Trạng thái INFLATE2
void inflate2State() {
    float pressure = readPressure(true);
    if (pressure >= maxPressure) states.stopCount++;
    else states.stopCount = 0;

    if (states.stopCount >= 5) {
        digitalWrite(MOTOR_PIN, OFF);
        delay(1000);
        states.currentState = DEFLATE_STATE;
        timers.timedeflate = 0;
    }
}

// Trạng thái DEFLATE
void deflateState() {
    pressureMeasure();
}

// Trạng thái DISPLAY
void displayState() {
    if (timers.timedisplay <= 3000) {
        if (timers.timecount >= 200) {
            timers.timecount = 0;
            measurements.bpMeasured = true;
        }
    } else {
        timers.timedisplay = 0;
        states.currentState = START_STATE; // Quay lại START thay vì STOP
    }
}

// Trạng thái RESET
void resetState() {
    if (timers.timedisplay > 1000 && timers.timedisplay < 2000 && timers.timecount >= 200) {
        timers.timecount = 0;
    } else if (timers.timedisplay >= 2000) {
        timers.timedisplay = 0;
    }
}

// Đo áp suất và tính toán
void pressureMeasure() {
    if (timers.timing != 0) return;

    float pressure, adc;
    long data = readHX710B();
    adc = (((float)data) / 8388607.0) * 5.0 - 0.41;
    pressure = (adc / 128) / 0.05 * 300;

    switch (states.measState) {
        case SYS_MEASURE:
            if ((former <= thresholds.sys && adc > thresholds.sys) || 
                (former <= thresholds.sys2 && adc > thresholds.sys2)) {
                states.sysCount++;
            }
            former = adc;
            if (states.sysCount >= 2) states.measState = SYS_CAL;
            break;

        case SYS_CAL:
            if (states.count < 4) {
                states.count++;
                adc += adc; // Tích lũy dữ liệu
            } else {
                measurements.systolic = (adc / 4 / 128) * 6000;
                states.measState = RATE_MEASURE;
                states.count = states.countAverage = 0;
                former = 1.9;
            }
            break;

        case RATE_MEASURE:
            if (states.countAverage < 2) {
                if ((former < thresholds.rate && adc > thresholds.rate) || 
                    (former < thresholds.rate2 && adc > thresholds.rate2)) {
                    timers.timerate = 0;
                    states.countPulse = 1;
                    former = adc;
                    states.countAverage++;
                }
            } else {
                float pulsePeriod = (float)timers.timerate / 5000;
                measurements.pulse_per_min = 60 / pulsePeriod;
                states.measState = DIAS_MEASURE;
                states.countAverage = 0;
                timers.timerun_dias = 0;
            }
            former = adc;
            break;

        case DIAS_MEASURE:
            if (timers.timerun_dias < 2000 && adc > thresholds.dias) {
                timers.timerun_dias = 0;
            } else if (timers.timerun_dias >= 2000) {
                states.measState = DIAS_CAL;
            }
            break;

        case DIAS_CAL:
            measurements.diastolic = (adc / 128) * 6000;
            states.measState = SYS_MEASURE;
            states.currentState = DISPLAY_STATE;
            digitalWrite(VALVE_PIN, ON);
            break;
    }
    timers.timing = 40; // Đặt lại thời gian lấy mẫu
}

// Vòng lặp chính
void loop() {
    receiveCommandFromESP();
    debounceButton();

    bool isRunning = isPhysicalRunning || isRemoteRunning;
    if (isRunning) {
        switch (states.currentState) {
            case START_STATE: startState(); break;
            case INFLATE2_STATE: inflate2State(); break;
            case DEFLATE_STATE: deflateState(); break;
            case DISPLAY_STATE: displayState(); break;
            case RESET_STATE: resetState(); break;
        }
    }

    if (measurements.bpMeasured && (millis() - lastBpSendTime >= bpSendInterval)) {
        lastBpSendTime = millis();
        sendBloodPressureData();
    }
}