#include <Arduino.h>
#include "Globals.h"


void setup() {
  Serial.begin(115200);
  pinMode(BUILTIN_LED, OUTPUT);

  xTaskCreate(appLinkInit, "appLink", 4096, NULL, 0, NULL);

}

void loop() {
  delay(1);
}
