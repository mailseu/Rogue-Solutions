#include <Arduino.h>
#include <Wire.h>
#include "DFRobot_HuskylensV2.h"

const int HUSKY_SDA = 33;
const int HUSKY_SCL = 22;

HuskylensV2 huskylens;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("Starting ESP32 + HuskyLens Tag Recognition...");
  Wire.begin(HUSKY_SDA, HUSKY_SCL);

  while (!huskylens.begin(Wire)) {
    Serial.println("HuskyLens not detected. Check wiring and mode.");
    delay(1000);
  }
  
  Serial.println("HuskyLens connected!");
  huskylens.switchAlgorithm(ALGORITHM_TAG_RECOGNITION);
  Serial.println("Switched to TAG RECOGNITION mode.");
}

void loop() {
  huskylens.getResult(ALGORITHM_TAG_RECOGNITION);

  if (huskylens.available(ALGORITHM_TAG_RECOGNITION)) {
    Serial.print("Total number of tags: ");
    Serial.println(huskylens.getCachedResultNum(ALGORITHM_TAG_RECOGNITION));

    auto centerResult = huskylens.getCachedCenterResult(ALGORITHM_TAG_RECOGNITION);

    Serial.print("ID of the tag near the center: ");
    Serial.println(RET_ITEM_NUM(centerResult, Result, ID));

    Serial.print("Center coordinates of the tag near the center: ");
    Serial.print(RET_ITEM_NUM(centerResult, Result, xCenter));
    Serial.print(", ");
    Serial.println(RET_ITEM_NUM(centerResult, Result, yCenter));

    auto firstResult = huskylens.getCachedResultByIndex(ALGORITHM_TAG_RECOGNITION, 0);

    if (firstResult != NULL) {
      Serial.print("ID of the first detected tag: ");
      Serial.println(RET_ITEM_NUM(firstResult, Result, ID));
    }
  } else {
    Serial.println("No tags detected.");
  }

  delay(500);
}