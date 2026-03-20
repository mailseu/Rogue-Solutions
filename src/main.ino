#include <Arduino.h>

const int RPWM = 5;
const int LPWM = 18;
const int LEN  = 19;
const int REN  = 21;

const int chRPWM = 0;
const int chLPWM = 1;

void setup() {
    Serial.begin(115200);

    pinMode(RPWM, OUTPUT);
    pinMode(LPWM, OUTPUT);
    pinMode(LEN, OUTPUT);
    pinMode(REN, OUTPUT);

    // Start everything low
    digitalWrite(RPWM, LOW);
    digitalWrite(LPWM, LOW);
    digitalWrite(LEN, LOW);
    digitalWrite(REN, LOW);

    delay(100);

    // Setup PWM on ESP32
    ledcSetup(chRPWM, 1000, 8);
    ledcSetup(chLPWM, 1000, 8);

    ledcAttachPin(RPWM, chRPWM);
    ledcAttachPin(LPWM, chLPWM);

    // Enable both sides of BTS7960
    digitalWrite(LEN, HIGH);
    digitalWrite(REN, HIGH);

    // Motor off at startup
    ledcWrite(chRPWM, 0);
    ledcWrite(chLPWM, 0);

    Serial.println("Motor driver enabled");
}

void loop() {
    // Motor ON forward
    ledcWrite(chLPWM, 0);
    ledcWrite(chRPWM, 255);
    Serial.println("Motor ON");
    delay(3000);

    // Motor OFF
    ledcWrite(chRPWM, 0);
    ledcWrite(chLPWM, 0);
    Serial.println("Motor OFF");
    delay(3000);
}