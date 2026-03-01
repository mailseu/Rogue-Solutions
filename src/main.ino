#include <Arduino.h>
#include "blink.h"

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    blinkOnce(LED_BUILTIN);   // <-- PASS THE PIN
}