#include <Arduino.h>
#include "blink_esp32.h"

void blinkOnce_esp32(byte pin) {
  digitalWrite(pin, HIGH);
  delay(1000);
    
  digitalWrite(pin, LOW);
  delay(1000);
}