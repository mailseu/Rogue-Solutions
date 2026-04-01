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
const int CH1_PIN = 34;   // steering (CH1)
const int CH2_PIN = 35;   // throttle (CH2)

// PWM channels
const int chRPWM1 = 0;
const int chLPWM1 = 1;
const int chRPWM2 = 2;
const int chLPWM2 = 3;

void setup() {
     Serial.begin(115200);

    // Motor outputs
    pinMode(RPWM1, OUTPUT);
    pinMode(LPWM1, OUTPUT);
    pinMode(LEN1, OUTPUT);
    pinMode(REN1, OUTPUT);

    pinMode(RPWM2, OUTPUT);
    pinMode(LPWM2, OUTPUT);
    pinMode(LEN2, OUTPUT);
    pinMode(REN2, OUTPUT);

    // Receiver inputs
    pinMode(CH1_PIN, INPUT);
    pinMode(CH2_PIN, INPUT);

    // Start low
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

    // Enable both BTS7960 boards
    digitalWrite(LEN1, HIGH);
    digitalWrite(REN1, HIGH);
    digitalWrite(LEN2, HIGH);
    digitalWrite(REN2, HIGH);

    // Motors off
    ledcWrite(chRPWM1, 0);
    ledcWrite(chLPWM1, 0);
    ledcWrite(chRPWM2, 0);
    ledcWrite(chLPWM2, 0);

    Serial.println("Receiver + motor drivers ready");
}

void loop() {
    // Read receiver PWM pulses (typical RC: 1000 to 2000 us)
    int ch1 = pulseIn(CH1_PIN, HIGH, 25000);   // steering
    int ch2 = pulseIn(CH2_PIN, HIGH, 25000);   // throttle

    // Failsafe if no signal
    if (ch1 == 0 || ch2 == 0) {
        ledcWrite(chRPWM1, 0);
        ledcWrite(chLPWM1, 0);
        ledcWrite(chRPWM2, 0);
        ledcWrite(chLPWM2, 0);

        Serial.println("No receiver signal");
        delay(50);
        return;
    }

    // Dead zones around stick center
    if (ch1 > 1470 && ch1 < 1530) ch1 = 1500;
    if (ch2 > 1470 && ch2 < 1530) ch2 = 1500;

    // Convert throttle and steering to -255 .. 255
    int throttle = map(ch2, 1000, 2000, -255, 255);
    int steering = map(ch1, 1000, 2000, -255, 255);

    // Mix for tank steering
    int leftMotor  = throttle + steering;
    int rightMotor = throttle - steering;

    // Limit range
    if (leftMotor > 255) leftMotor = 255;
    if (leftMotor < -255) leftMotor = -255;
    if (rightMotor > 255) rightMotor = 255;
    if (rightMotor < -255) rightMotor = -255;

    // LEFT MOTOR (Motor 1)
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

    // RIGHT MOTOR (Motor 2)
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

    // Serial debug
    Serial.print("CH1: ");
    Serial.print(ch1);
    Serial.print("  CH2: ");
    Serial.print(ch2);
    Serial.print("  Left: ");
    Serial.print(leftMotor);
    Serial.print("  Right: ");
    Serial.println(rightMotor);

    delay(20);
}