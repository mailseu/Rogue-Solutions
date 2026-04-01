#include <Arduino.h>

// Motor 1 pins
const int RPWM1 = 5;
const int LPWM1 = 18;
const int LEN1  = 19;
const int REN1  = 21;

// Motor 2 pins (other side of ESP32)
const int RPWM2 = 25;
const int LPWM2 = 26;
const int LEN2  = 27;
const int REN2  = 14;

// PWM channels
const int chRPWM1 = 0;
const int chLPWM1 = 1;
const int chRPWM2 = 2;
const int chLPWM2 = 3;

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

    // Start LOW
    digitalWrite(RPWM1, LOW);
    digitalWrite(LPWM1, LOW);
    digitalWrite(LEN1, LOW);
    digitalWrite(REN1, LOW);

    digitalWrite(RPWM2, LOW);
    digitalWrite(LPWM2, LOW);
    digitalWrite(LEN2, LOW);
    digitalWrite(REN2, LOW);

    delay(100);

    // PWM setup
    ledcSetup(chRPWM1, 1000, 8);
    ledcSetup(chLPWM1, 1000, 8);
    ledcSetup(chRPWM2, 1000, 8);
    ledcSetup(chLPWM2, 1000, 8);

    ledcAttachPin(RPWM1, chRPWM1);
    ledcAttachPin(LPWM1, chLPWM1);
    ledcAttachPin(RPWM2, chRPWM2);
    ledcAttachPin(LPWM2, chLPWM2);

    // Enable drivers
    digitalWrite(LEN1, HIGH);
    digitalWrite(REN1, HIGH);
    digitalWrite(LEN2, HIGH);
    digitalWrite(REN2, HIGH);

    // Motors OFF
    ledcWrite(chRPWM1, 0);
    ledcWrite(chLPWM1, 0);
    ledcWrite(chRPWM2, 0);
    ledcWrite(chLPWM2, 0);

    Serial.println("Both motor drivers enabled");
}

void loop() {
    ledcWrite(chLPWM1, 0);
    ledcWrite(chRPWM1, 255);
    ledcWrite(chLPWM2, 0);
    ledcWrite(chRPWM2, 255);

    Serial.println("Both motors forward");
    delay(3000);

    ledcWrite(chRPWM1, 0);
    ledcWrite(chLPWM1, 0);
    ledcWrite(chRPWM2, 0);
    ledcWrite(chLPWM2, 0);

    Serial.println("Both motors OFF");
    delay(2000);

    ledcWrite(chRPWM1, 0);
    ledcWrite(chLPWM1, 127);
    ledcWrite(chRPWM2, 0);
    ledcWrite(chLPWM2, 127);

    Serial.println("Both motors reverse 50%");
    delay(3000);

    ledcWrite(chRPWM1, 0);
    ledcWrite(chLPWM1, 0);
    ledcWrite(chRPWM2, 0);
    ledcWrite(chLPWM2, 0);

    Serial.println("Both motors OFF");
    delay(2000);
}