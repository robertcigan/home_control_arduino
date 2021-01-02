#include "src/RedMP3/RedMP3.h"
#include "src/SimpleTimer/SimpleTimer.h"

#include "HomeControl.h"
#include "Wire.h"

HomeControl home_control;

void setup_serial() {
  Serial.begin(9600);
  Serial.println(F(""));
  Serial.println(F("HomeControl "));
  Serial.print(F("Version: "));
  Serial.println(VERSION);
  Serial.println(F(""));
}

void setup() {
  Wire.begin();
  setup_serial();
  home_control.setup();
}

void loop() {
  home_control.loop();  
}
