#include "HomeControl.h"
#include "Wire.h"

HomeControl home_control;

void setup() {
  Wire.begin();
  Serial.begin(115200);\
  Serial.println(F("Home Control loading..."));
  home_control.setup();
}

void loop() {
  home_control.loop();  
}