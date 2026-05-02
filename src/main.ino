#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "DFRobot_HuskylensV2.h"

// Motor 1 pins
const int RPWM1 = 23;
const int LPWM1 = 18;
const int LEN1  = 21;
const int REN1  = 19;

// Motor 2 pins
const int RPWM2 = 25;
const int LPWM2 = 26;
const int LEN2  = 27;
const int REN2  = 14;

// Receiver pins
const int CH1_PIN = 34;   // Steering
const int CH2_PIN = 35;   // Throttle
const int CH5_PIN = 39;   // Arm Switch
const int CH6_PIN = 13;   // Mode Switch: Manual / Autonomous

// Battery monitor pin
const int BATTERY_PIN = 32;   // Voltage divider output to ADC

// PWM channels
const int chRPWM1 = 0;
const int chLPWM1 = 1;
const int chRPWM2 = 2;
const int chLPWM2 = 3;

//HuskeyLens 2 AI Vision Camrea
const int HUSKY_SDA = 33;
const int HUSKY_SCL = 22;

HuskylensV2 huskylens;

volatile bool liveTagDetected = false;
volatile int liveTagID = 0;
volatile int liveTagX = 0;
volatile int liveTagY = 0;

// Phone hotspot settings
const char* ssid = "FrontMesh4608";
const char* password = "vjtn9fk4965g37lg";

WebServer server(80);

// Battery divider calibration
const float R1 = 100000.0;   // 100k
const float R2 = 22000.0;    // 22k
const float DIVIDER_RATIO = (R1 + R2) / R2;

const float ADC_REF = 3.3;
const int ADC_MAX = 4095;

float BATTERY_CAL = 1.037;
const int CELL_COUNT = 4;

// Battery smoothing
float filteredPackVoltage = 0.0;
bool batteryFilterInitialized = false;
unsigned long lastBatteryUpdate = 0;
const unsigned long batteryInterval = 200;   // ms

// Values for USB serial & Web page     
volatile int liveCH1 = 0;
volatile int liveCH2 = 0;
volatile int liveCH5 = 0;
volatile int liveCH6 = 0;
volatile int liveLeft = 0;
volatile int liveRight = 0;
volatile bool liveArmed = false;
volatile bool liveSignalOK = false;
volatile bool liveAutoMode = false;
volatile float liveBatteryPack = 0.0;
volatile float liveBatteryCell = 0.0;
volatile int liveBatteryPercent = 0;

void stopMotors() {
    ledcWrite(chRPWM1, 0);
    ledcWrite(chLPWM1, 0);
    ledcWrite(chRPWM2, 0);
    ledcWrite(chLPWM2, 0);
    liveLeft = 0;
    liveRight = 0;
}

void setMotors(int leftMotor, int rightMotor) {
    if (leftMotor > 255) leftMotor = 255;
    if (leftMotor < -255) leftMotor = -255;
    if (rightMotor > 255) rightMotor = 255;
    if (rightMotor < -255) rightMotor = -255;

    liveLeft = leftMotor;
    liveRight = rightMotor;

    // LEFT MOTOR = normal
    if (leftMotor > 10) {
        ledcWrite(chLPWM1, 0);
        ledcWrite(chRPWM1, leftMotor);
    } else if (leftMotor < -10) {
        ledcWrite(chRPWM1, 0);
        ledcWrite(chLPWM1, -leftMotor);
    } else {
        ledcWrite(chRPWM1, 0);
        ledcWrite(chLPWM1, 0);
    }

    // RIGHT MOTOR = inverted
    if (rightMotor > 10) {
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, rightMotor);
    } else if (rightMotor < -10) {
        ledcWrite(chLPWM2, 0);
        ledcWrite(chRPWM2, -rightMotor);
    } else {
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, 0);
    }
}

float readBatteryVoltage() {
    long sum = 0;
    const int samples = 80;

    for (int i = 0; i < samples; i++) {
        sum += analogRead(BATTERY_PIN);
        delay(1);
    }

    float raw = sum / (float)samples;
    float pinVoltage = (raw / ADC_MAX) * ADC_REF;
    float packVoltage = pinVoltage * DIVIDER_RATIO * BATTERY_CAL;

    return packVoltage;
}

int batteryPercentFromCellVoltage(float cellVoltage) {
    if (cellVoltage < 3.3) cellVoltage = 3.3;
    if (cellVoltage > 4.2) cellVoltage = 4.2;

    float percent = ((cellVoltage - 3.3) / (4.2 - 3.3)) * 100.0;

    if (percent < 0.0) percent = 0.0;
    if (percent > 100.0) percent = 100.0;

    return (int)(percent + 0.5);
}

void updateBatteryReadings() {
    float packVoltage = readBatteryVoltage();
    const float alpha = 0.05; // Lower alpha = more stable, slower response (Battery Percentage)

    if (!batteryFilterInitialized) {
        filteredPackVoltage = packVoltage;
        batteryFilterInitialized = true;
    } else {
        filteredPackVoltage = (alpha * packVoltage) + ((1.0 - alpha) * filteredPackVoltage);
    }

    float cellVoltage = filteredPackVoltage / CELL_COUNT;

    if (cellVoltage < 3.3) cellVoltage = 3.3;
    if (cellVoltage > 4.2) cellVoltage = 4.2;

    liveBatteryPack = filteredPackVoltage;
    liveBatteryCell = cellVoltage;
    liveBatteryPercent = batteryPercentFromCellVoltage(cellVoltage);
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to hotspot");
    int tries = 0;

    while (WiFi.status() != WL_CONNECTED && tries < 30) {
        delay(500);
        Serial.print(".");
        tries++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to hotspot");
        Serial.print("ESP32 IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Open browser to: http://");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Failed to connect to hotspot");
    }
}

void setup() {
    Serial.begin(115200);

    Wire.begin(HUSKY_SDA, HUSKY_SCL);

    Serial.println("Starting HuskyLens...");

    while (!huskylens.begin(Wire)) {
        Serial.println("HuskyLens not detected. Check SDA/SCL, power, and protocol.");
        delay(1000);
    }

    Serial.println("HuskyLens connected!");
    huskylens.switchAlgorithm(ALGORITHM_TAG_RECOGNITION);
    Serial.println("HuskyLens set to TAG RECOGNITION mode.");

    pinMode(RPWM1, OUTPUT);
    pinMode(LPWM1, OUTPUT);
    pinMode(LEN1, OUTPUT);
    pinMode(REN1, OUTPUT);

    pinMode(RPWM2, OUTPUT);
    pinMode(LPWM2, OUTPUT);
    pinMode(LEN2, OUTPUT);
    pinMode(REN2, OUTPUT);

    pinMode(CH1_PIN, INPUT);
    pinMode(CH2_PIN, INPUT);
    pinMode(CH5_PIN, INPUT);
    pinMode(CH6_PIN, INPUT);

    pinMode(BATTERY_PIN, INPUT);

    digitalWrite(RPWM1, LOW);
    digitalWrite(LPWM1, LOW);
    digitalWrite(LEN1, LOW);
    digitalWrite(REN1, LOW);

    digitalWrite(RPWM2, LOW);
    digitalWrite(LPWM2, LOW);
    digitalWrite(LEN2, LOW);
    digitalWrite(REN2, LOW);

    delay(100);

    ledcSetup(chRPWM1, 1000, 8);
    ledcSetup(chLPWM1, 1000, 8);
    ledcSetup(chRPWM2, 1000, 8);
    ledcSetup(chLPWM2, 1000, 8);

    ledcAttachPin(RPWM1, chRPWM1);
    ledcAttachPin(LPWM1, chLPWM1);
    ledcAttachPin(RPWM2, chRPWM2);
    ledcAttachPin(LPWM2, chLPWM2);

    digitalWrite(LEN1, HIGH);
    digitalWrite(REN1, HIGH);
    digitalWrite(LEN2, HIGH);
    digitalWrite(REN2, HIGH);

    analogReadResolution(12);
    stopMotors();
    updateBatteryReadings();

    Serial.println("System ready");
    Serial.println("===== PIN SETUP =====");
    Serial.print("Motor 1 -> RPWM1: ");
    Serial.print(RPWM1);
    Serial.print("  LPWM1: ");
    Serial.print(LPWM1);
    Serial.print("  LEN1: ");
    Serial.print(LEN1);
    Serial.print("  REN1: ");
    Serial.println(REN1);

    Serial.print("Motor 2 -> RPWM2: ");
    Serial.print(RPWM2);
    Serial.print("  LPWM2: ");
    Serial.print(LPWM2);
    Serial.print("  LEN2: ");
    Serial.print(LEN2);
    Serial.print("  REN2: ");
    Serial.println(REN2);

    Serial.print("RX -> CH1_PIN: ");
    Serial.print(CH1_PIN);
    Serial.print("  CH2_PIN: ");
    Serial.print(CH2_PIN);
    Serial.print("  CH5_PIN: ");
    Serial.print(CH5_PIN);
    Serial.print("  CH6_PIN: ");
    Serial.println(CH6_PIN);

    Serial.print("Battery ADC -> BATTERY_PIN: ");
    Serial.println(BATTERY_PIN);

    Serial.print("PWM Channels -> chRPWM1: ");
    Serial.print(chRPWM1);
    Serial.print("  chLPWM1: ");
    Serial.print(chLPWM1);
    Serial.print("  chRPWM2: ");
    Serial.print(chRPWM2);
    Serial.print("  chLPWM2: ");
    Serial.println(chLPWM2);
    Serial.println("=====================");

    connectWiFi();

    server.on("/", []() {
        String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>9 LIVES Rover Monitor</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #111; color: #eee; }
        h1 { margin-bottom: 8px; }
        .card { background: #1c1c1c; border-radius: 12px; padding: 16px; margin-bottom: 14px; }
        .row { margin: 8px 0; font-size: 18px; }
        .label { color: #aaa; display: inline-block; width: 140px; }
        .ok { color: #5f5; font-weight: bold; }
        .bad { color: #f66; font-weight: bold; }
        .mode { color: #6cf; font-weight: bold; }
        .battery { color: #ffd166; font-weight: bold; }
        code { color: #8fd; }
    </style>
    </head>
    <body>
    <h1>9 LIVES Rover Monitor</h1>

    <div class="card">
        <div class="row"><span class="label">Arm state</span><span id="armed">-</span></div>
        <div class="row"><span class="label">Signal</span><span id="signal">-</span></div>
        <div class="row"><span class="label">Mode</span><span id="mode">-</span></div>
    </div>

    <div class="card">
        <div class="row"><span class="label">Pack voltage</span><span id="battPack" class="battery">0.00 V</span></div>
        <div class="row"><span class="label">Per cell</span><span id="battCell" class="battery">0.00 V</span></div>
        <div class="row"><span class="label">Battery</span><span id="battPercent" class="battery">0%</span></div>
    </div>

    <div class="card">
        <div class="row"><span class="label">CH1</span><span id="ch1">0</span></div>
        <div class="row"><span class="label">CH2</span><span id="ch2">0</span></div>
        <div class="row"><span class="label">CH5</span><span id="ch5">0</span></div>
        <div class="row"><span class="label">CH6</span><span id="ch6">0</span></div>
        <div class="row"><span class="label">Left motor</span><span id="left">0</span></div>
        <div class="row"><span class="label">Right motor</span><span id="right">0</span></div>
    </div>

    <div class="card">
        <div class="row"><span class="label">Tag detected</span><span id="tagDetected">-</span></div>
        <div class="row"><span class="label">Tag ID</span><span id="tagID">0</span></div>
        <div class="row"><span class="label">Tag X</span><span id="tagX">0</span></div>
        <div class="row"><span class="label">Tag Y</span><span id="tagY">0</span></div>
    </div>

    <script>
        async function updateData() {
        try {
            const r = await fetch('/data');
            const d = await r.json();

            document.getElementById('ch1').textContent = d.ch1;
            document.getElementById('ch2').textContent = d.ch2;
            document.getElementById('ch5').textContent = d.ch5;
            document.getElementById('ch6').textContent = d.ch6;
            document.getElementById('left').textContent = d.left;
            document.getElementById('right').textContent = d.right;

            document.getElementById('battPack').textContent = d.battPack.toFixed(2) + ' V';
            document.getElementById('battCell').textContent = d.battCell.toFixed(3) + ' V';
            document.getElementById('battPercent').textContent = d.battPercent + '%';

            document.getElementById('tagDetected').textContent = d.tagDetected ? 'YES' : 'NO';
            document.getElementById('tagID').textContent = d.tagID;
            document.getElementById('tagX').textContent = d.tagX;
            document.getElementById('tagY').textContent = d.tagY;

            const armed = document.getElementById('armed');
            armed.textContent = d.armed ? 'ARMED' : 'DISARMED';
            armed.className = d.armed ? 'ok' : 'bad';

            const signal = document.getElementById('signal');
            signal.textContent = d.signal ? 'OK' : 'NO SIGNAL';
            signal.className = d.signal ? 'ok' : 'bad';

            const mode = document.getElementById('mode');
            mode.textContent = d.auto ? 'AUTONOMOUS' : 'MANUAL';
            mode.className = 'mode';
        } catch (e) {
            const signal = document.getElementById('signal');
            signal.textContent = 'PAGE LOST CONNECTION';
            signal.className = 'bad';
        }
        }

        setInterval(updateData, 500);
        updateData();
    </script>
    </body>
    </html>
    )rawliteral";

        server.send(200, "text/html", html);
    });

    server.on("/data", []() {
        String json = "{";
        json += "\"ch1\":" + String(liveCH1) + ",";
        json += "\"ch2\":" + String(liveCH2) + ",";
        json += "\"ch5\":" + String(liveCH5) + ",";
        json += "\"ch6\":" + String(liveCH6) + ",";
        json += "\"left\":" + String(liveLeft) + ",";
        json += "\"right\":" + String(liveRight) + ",";
        json += "\"armed\":" + String(liveArmed ? "true" : "false") + ",";
        json += "\"signal\":" + String(liveSignalOK ? "true" : "false") + ",";
        json += "\"auto\":" + String(liveAutoMode ? "true" : "false") + ",";
        json += "\"battPack\":" + String(liveBatteryPack, 2) + ",";
        json += "\"battCell\":" + String(liveBatteryCell, 3) + ",";
        json += "\"battPercent\":" + String(liveBatteryPercent) + ",";
        json += "\"tagDetected\":" + String(liveTagDetected ? "true" : "false") + ",";
        json += "\"tagID\":" + String(liveTagID) + ",";
        json += "\"tagX\":" + String(liveTagX) + ",";
        json += "\"tagY\":" + String(liveTagY);
        json += "}";
        server.send(200, "application/json", json);
    });

    server.begin();
    Serial.println("Web server started");
}

void loop() {
    server.handleClient();

    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (millis() - lastBatteryUpdate >= batteryInterval) {
        lastBatteryUpdate = millis();
        updateBatteryReadings();
    }

    int ch1 = pulseIn(CH1_PIN, HIGH, 50000);
    int ch2 = pulseIn(CH2_PIN, HIGH, 50000);
    int ch5 = pulseIn(CH5_PIN, HIGH, 50000);
    int ch6 = pulseIn(CH6_PIN, HIGH, 50000);

    liveCH1 = ch1;
    liveCH2 = ch2;
    liveCH5 = ch5;
    liveCH6 = ch6;

    if (ch1 == 0 || ch2 == 0 || ch5 == 0 || ch6 == 0) { //Signal Check
        stopMotors();
        liveArmed = false;
        liveSignalOK = false;
        liveAutoMode = false;

        liveTagDetected = false;
        liveTagID = 0;
        liveTagX = 0;
        liveTagY = 0;

        Serial.print("No signal | Pack: ");
        Serial.print(liveBatteryPack, 2);
        Serial.print(" V | Cell: ");
        Serial.print(liveBatteryCell, 3);
        Serial.print(" V | Battery: ");
        Serial.print(liveBatteryPercent);
        Serial.println("%");

        delay(50);
        return;
    }

    liveSignalOK = true;

    if (ch5 < 1500) { //Disarmed Check
        stopMotors();
        liveArmed = false;

        liveTagDetected = false;
        liveTagID = 0;
        liveTagX = 0;
        liveTagY = 0;

        Serial.print("DISARMED | Pack: ");
        Serial.print(liveBatteryPack, 2);
        Serial.print(" V | Cell: ");
        Serial.print(liveBatteryCell, 3);
        Serial.print(" V | Battery: ");
        Serial.print(liveBatteryPercent);
        Serial.println("%");

        delay(50);
        return;
    }

    liveArmed = true;

    // Mode switch
    liveAutoMode = (ch6 > 1500);

    if (liveAutoMode) {
        huskylens.getResult(ALGORITHM_TAG_RECOGNITION);

        if (huskylens.available(ALGORITHM_TAG_RECOGNITION)) {
            auto centerResult = huskylens.getCachedCenterResult(ALGORITHM_TAG_RECOGNITION);

            int tagID = RET_ITEM_NUM(centerResult, Result, ID);
            int tagX  = RET_ITEM_NUM(centerResult, Result, xCenter);
            int tagY  = RET_ITEM_NUM(centerResult, Result, yCenter);

            liveTagDetected = true;
            liveTagID = tagID;
            liveTagX = tagX;
            liveTagY = tagY;

            int targetX = 160;
            int error = tagX - targetX;

            int baseSpeed = 80;
            float turnGain = 0.6;
            int deadband = 25;

            int turn = 0;

            if (abs(error) > deadband) {
                turn = error * turnGain;
            }

            if (turn > 90) turn = 90;
            if (turn < -90) turn = -90;

            int autoLeft = baseSpeed + turn;
            int autoRight = baseSpeed - turn;

            setMotors(autoLeft, autoRight);

            Serial.print("AUTO TRACK | Tag ID: ");
            Serial.print(tagID);
            Serial.print(" X: ");
            Serial.print(tagX);
            Serial.print(" Y: ");
            Serial.print(tagY);
            Serial.print(" Error: ");
            Serial.print(error);
            Serial.print(" | L: ");
            Serial.print(autoLeft);
            Serial.print(" R: ");
            Serial.println(autoRight);
        } 
        else {
            liveTagDetected = false;
            liveTagID = 0;
            liveTagX = 0;
            liveTagY = 0;

            stopMotors();

            Serial.println("AUTO MODE | No tag detected. Motors stopped.");
        }

        delay(50);
        return;
    }

    // ===== MANUAL MODE =====

    if (ch1 > 1470 && ch1 < 1530) ch1 = 1500;
    if (ch2 > 1470 && ch2 < 1530) ch2 = 1500;

    int throttle = map(ch2, 1000, 2000, -255, 255);
    int steering = map(ch1, 1000, 2000, -255, 255);

    int leftMotor  = throttle + steering;
    int rightMotor = throttle - steering;

    if (leftMotor > 255) leftMotor = 255;
    if (leftMotor < -255) leftMotor = -255;
    if (rightMotor > 255) rightMotor = 255;
    if (rightMotor < -255) rightMotor = -255;

    setMotors(leftMotor, rightMotor);

    Serial.print("MANUAL MODE | CH1: ");
    Serial.print(ch1);
    Serial.print(" CH2: ");
    Serial.print(ch2);
    Serial.print(" CH5: ");
    Serial.print(ch5);
    Serial.print(" CH6: ");
    Serial.print(ch6);
    Serial.print(" | L: ");
    Serial.print(leftMotor);
    Serial.print(" R: ");
    Serial.print(rightMotor);
    Serial.print(" | Pack: ");
    Serial.print(liveBatteryPack, 2);
    Serial.print(" V | Cell: ");
    Serial.print(liveBatteryCell, 3);
    Serial.print(" V | Battery: ");
    Serial.print(liveBatteryPercent);
    Serial.println("%");

    delay(50);
}