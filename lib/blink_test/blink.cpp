#include <Arduino.h>
#include "blink.h"

void blinkOnce(byte pin) {
  digitalWrite(pin, HIGH);
  delay(100);

  digitalWrite(pin, LOW);
  delay(100);
}