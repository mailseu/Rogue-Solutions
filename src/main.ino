#include <Arduino.h>

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
const int CH1_PIN = 34;   // steering or right motor (CH1)
const int CH2_PIN = 35;   // throttle or left motor (CH2)
const int CH5_PIN = 39;   // SWA mode switch (CH3)

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

    Serial.println("Receiver + motor drivers ready");
}

void loop() {
    int ch1 = pulseIn(CH1_PIN, HIGH, 50000);
    int ch2 = pulseIn(CH2_PIN, HIGH, 50000);
    int ch5 = pulseIn(CH5_PIN, HIGH, 50000);

    // No Signal Check
    if (ch1 == 0 || ch2 == 0 || ch5 == 0) {
        ledcWrite(chRPWM1, 0);
        ledcWrite(chLPWM1, 0);
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, 0);

        Serial.println("No signal");
        delay(50);
        return;
    }

    // ARM SWITCH (CH5)
    if (ch5 < 1500) {
        ledcWrite(chRPWM1, 0);
        ledcWrite(chLPWM1, 0);
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, 0);

        Serial.println("DISARMED");
        delay(100);
        return;
    }

    // Input PWM Data (ONLY WHEN ARMED)
    Serial.print("CH1: ");
    Serial.print(ch1);
    Serial.print("  CH2: ");
    Serial.print(ch2);
    Serial.print("  CH5: ");
    Serial.print(ch5);

    // Deadzone
    if (ch1 > 1470 && ch1 < 1530) ch1 = 1500;
    if (ch2 > 1470 && ch2 < 1530) ch2 = 1500;

    int throttle = map(ch2, 1000, 2000, -255, 255);
    int steering = map(ch1, 1000, 2000, -255, 255);

    int leftMotor  = throttle + steering;
    int rightMotor = throttle - steering;

    // Controller Limit
    if (leftMotor > 255) leftMotor = 255;
    if (leftMotor < -255) leftMotor = -255;
    if (rightMotor > 255) rightMotor = 255;
    if (rightMotor < -255) rightMotor = -255;

    // Motor 1 Control
    if (leftMotor > 10) {
        ledcWrite(chLPWM1, 0);
        ledcWrite(chRPWM1, leftMotor);
    } 
    else if (leftMotor < -10) {
        ledcWrite(chRPWM1, 0);
        ledcWrite(chLPWM1, -leftMotor);
    } 
    else {
        ledcWrite(chRPWM1, 0);
        ledcWrite(chLPWM1, 0);
    }

    // Motor 2 Control
    if (rightMotor > 10) {
        ledcWrite(chLPWM2, 0);
        ledcWrite(chRPWM2, rightMotor);
    } 
    else if (rightMotor < -10) {
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, -rightMotor);
    } 
    else {
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, 0);
    }

    //Motor Output Data
    Serial.print("  | L: ");
    Serial.print(leftMotor);
    Serial.print("  R: ");
    Serial.println(rightMotor);

    delay(50);
}