#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// Motor 1 pins
const int RPWM1 = 5;
const int LPWM1 = 18;
const int LEN1  = 19;
const int REN1  = 21;

// Motor 2 pins
const int RPWM2 = 25;
const int LPWM2 = 26;
const int LEN2  = 27;
const int REN2  = 14;

// Receiver pins
const int CH1_PIN = 34;   // steering
const int CH2_PIN = 35;   // throttle
const int CH5_PIN = 39;   // SWA arm switch

// PWM channels
const int chRPWM1 = 0;
const int chLPWM1 = 1;
const int chRPWM2 = 2;
const int chLPWM2 = 3;

// Wi-Fi AP settings
const char* apName = "ESP32-Motor-Monitor";
const char* apPass = "12345678";

WebServer server(80);

// Live values for USB serial + web page
volatile int liveCH1 = 0;
volatile int liveCH2 = 0;
volatile int liveCH5 = 0;
volatile int liveLeft = 0;
volatile int liveRight = 0;
volatile bool liveArmed = false;
volatile bool liveSignalOK = false;

void setup() {
    Serial.begin(115200);

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

    ledcWrite(chRPWM1, 0);
    ledcWrite(chLPWM1, 0);
    ledcWrite(chRPWM2, 0);
    ledcWrite(chLPWM2, 0);

    Serial.println("System ready (ARM switch active)");
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
    Serial.println(CH5_PIN);

    Serial.print("PWM Channels -> chRPWM1: ");
    Serial.print(chRPWM1);
    Serial.print("  chLPWM1: ");
    Serial.print(chLPWM1);
    Serial.print("  chRPWM2: ");
    Serial.print(chRPWM2);
    Serial.print("  chLPWM2: ");
    Serial.println(chLPWM2);
    Serial.println("=====================");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName, apPass);
    IPAddress ip = WiFi.softAPIP();

    Serial.print("Wi-Fi AP ready. SSID: ");
    Serial.println(apName);
    Serial.print("Password: ");
    Serial.println(apPass);
    Serial.print("Open browser to: http://");
    Serial.println(ip);

    server.on("/", []() {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Motor Monitor</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background: #111; color: #eee; }
    h1 { margin-bottom: 8px; }
    .card { background: #1c1c1c; border-radius: 12px; padding: 16px; margin-bottom: 14px; }
    .row { margin: 8px 0; font-size: 18px; }
    .label { color: #aaa; display: inline-block; width: 120px; }
    .ok { color: #5f5; font-weight: bold; }
    .bad { color: #f66; font-weight: bold; }
    code { color: #8fd; }
  </style>
</head>
<body>
  <h1>ESP32 Motor Monitor</h1>
  <div class="card">
    <div class="row"><span class="label">Arm state</span><span id="armed">-</span></div>
    <div class="row"><span class="label">Signal</span><span id="signal">-</span></div>
  </div>

  <div class="card">
    <div class="row"><span class="label">CH1</span><span id="ch1">0</span></div>
    <div class="row"><span class="label">CH2</span><span id="ch2">0</span></div>
    <div class="row"><span class="label">CH5</span><span id="ch5">0</span></div>
    <div class="row"><span class="label">Left motor</span><span id="left">0</span></div>
    <div class="row"><span class="label">Right motor</span><span id="right">0</span></div>
  </div>

  <div class="card">
    <div class="row"><span class="label">Motor 1 pins</span><code>)rawliteral";
        html += "RPWM1=" + String(RPWM1) + " LPWM1=" + String(LPWM1) + " LEN1=" + String(LEN1) + " REN1=" + String(REN1);
        html += R"rawliteral(</code></div>
    <div class="row"><span class="label">Motor 2 pins</span><code>)rawliteral";
        html += "RPWM2=" + String(RPWM2) + " LPWM2=" + String(LPWM2) + " LEN2=" + String(LEN2) + " REN2=" + String(REN2);
        html += R"rawliteral(</code></div>
    <div class="row"><span class="label">RX pins</span><code>)rawliteral";
        html += "CH1=" + String(CH1_PIN) + " CH2=" + String(CH2_PIN) + " CH5=" + String(CH5_PIN);
        html += R"rawliteral(</code></div>
  </div>

  <script>
    async function updateData() {
      try {
        const r = await fetch('/data');
        const d = await r.json();

        document.getElementById('ch1').textContent = d.ch1;
        document.getElementById('ch2').textContent = d.ch2;
        document.getElementById('ch5').textContent = d.ch5;
        document.getElementById('left').textContent = d.left;
        document.getElementById('right').textContent = d.right;

        const armed = document.getElementById('armed');
        armed.textContent = d.armed ? 'ARMED' : 'DISARMED';
        armed.className = d.armed ? 'ok' : 'bad';

        const signal = document.getElementById('signal');
        signal.textContent = d.signal ? 'OK' : 'NO SIGNAL';
        signal.className = d.signal ? 'ok' : 'bad';
      } catch (e) {
        const signal = document.getElementById('signal');
        signal.textContent = 'PAGE LOST CONNECTION';
        signal.className = 'bad';
      }
    }

    setInterval(updateData, 200);
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
        json += "\"left\":" + String(liveLeft) + ",";
        json += "\"right\":" + String(liveRight) + ",";
        json += "\"armed\":" + String(liveArmed ? "true" : "false") + ",";
        json += "\"signal\":" + String(liveSignalOK ? "true" : "false");
        json += "}";
        server.send(200, "application/json", json);
    });

    server.begin();
    Serial.println("Web server started");
}

void loop() {
    server.handleClient();

    int ch1 = pulseIn(CH1_PIN, HIGH, 50000);
    int ch2 = pulseIn(CH2_PIN, HIGH, 50000);
    int ch5 = pulseIn(CH5_PIN, HIGH, 50000);

    liveCH1 = ch1;
    liveCH2 = ch2;
    liveCH5 = ch5;

    if (ch1 == 0 || ch2 == 0 || ch5 == 0) {
        ledcWrite(chRPWM1, 0);
        ledcWrite(chLPWM1, 0);
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, 0);

        liveLeft = 0;
        liveRight = 0;
        liveArmed = false;
        liveSignalOK = false;

        Serial.println("No signal");
        delay(50);
        return;
    }

    liveSignalOK = true;

    if (ch5 < 1500) {
        ledcWrite(chRPWM1, 0);
        ledcWrite(chLPWM1, 0);
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, 0);

        liveLeft = 0;
        liveRight = 0;
        liveArmed = false;

        Serial.println("DISARMED");
        delay(50);
        return;
    }

    liveArmed = true;

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

    liveLeft = leftMotor;
    liveRight = rightMotor;

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

    if (rightMotor > 10) {
        ledcWrite(chLPWM2, 0);
        ledcWrite(chRPWM2, rightMotor);
    } else if (rightMotor < -10) {
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, -rightMotor);
    } else {
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, 0);
    }

    Serial.print("CH1: ");
    Serial.print(ch1);
    Serial.print("  CH2: ");
    Serial.print(ch2);
    Serial.print("  CH5: ");
    Serial.print(ch5);
    Serial.print("  | L: ");
    Serial.print(leftMotor);
    Serial.print("  R: ");
    Serial.print(rightMotor);
    Serial.print("  | RPWM1: ");
    Serial.print(RPWM1);
    Serial.print("  LPWM1: ");
    Serial.print(LPWM1);
    Serial.print("  RPWM2: ");
    Serial.print(RPWM2);
    Serial.print("  LPWM2: ");
    Serial.println(LPWM2);

    delay(50);
}