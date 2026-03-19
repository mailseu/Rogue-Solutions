#include <Arduino.h>

const int RPWM = 5;
const int LPWM = 18;

const int chRPWM = 0;
const int chLPWM = 1;

const int pwmFreq = 1000;
const int pwmResolution = 8;

void motorForward(int speedVal);
void motorReverse(int speedVal);
void motorStop();

void setup() {
  Serial.begin(115200);

  // Force pins low first as plain GPIO
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  digitalWrite(RPWM, LOW);
  digitalWrite(LPWM, LOW);

  delay(200);

  // Setup PWM
  ledcSetup(chRPWM, pwmFreq, pwmResolution);
  ledcSetup(chLPWM, pwmFreq, pwmResolution);

  ledcAttachPin(RPWM, chRPWM);
  ledcAttachPin(LPWM, chLPWM);

  motorStop();
}

void loop() {
  motorStop();
  delay(1000);
}

void motorForward(int speedVal) {
  speedVal = constrain(speedVal, 0, 255);
  ledcWrite(chLPWM, 0);
  ledcWrite(chRPWM, speedVal);
}

void motorReverse(int speedVal) {
  speedVal = constrain(speedVal, 0, 255);
  ledcWrite(chRPWM, 0);
  ledcWrite(chLPWM, speedVal);
}

void motorStop() {
  ledcWrite(chRPWM, 0);
  ledcWrite(chLPWM, 0);
}