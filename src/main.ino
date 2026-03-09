#include <Arduino.h>
#include "blink_esp32.h"
#define LED 2

void setup() {
    pinMode(LED, OUTPUT);
}

void loop() {
    blinkOnce_esp32(LED);
}