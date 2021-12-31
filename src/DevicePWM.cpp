#include "DevicePWM.h"

DevicePWM::DevicePWM(uint32_t device_id, uint8_t pin, uint8_t default_value) {
  setup();
  this->pin = pin;
  this->device_id = device_id;
  this->value = default_value;
  // initialize output
  pinMode(pin, OUTPUT);
  #if defined(ESP32)
  #else
  analogWrite(pin, convertPercentToPWM(default_value));
  #endif
  print();
}

void DevicePWM::loop() {
}

void DevicePWM::set_pwm(uint8_t new_value) {
  if (new_value != value) {
    last_value_change = millis();
    value = new_value;
    #if defined(ESP32)
    #else
    analogWrite(pin, convertPercentToPWM(value));
    #endif
    #if defined(SHOW_VALUES_IN_SERIAL)
      Serial.print(F("Pin: ")); Serial.print(pin); Serial.print(F(" PWM set to ")); Serial.print(value); Serial.println(F("%"));
    #endif
  } 
}

void DevicePWM::uninitialize() { 
  #if defined(ESP32)
  #else
  analogWrite(pin, 0);
  #endif
}

void DevicePWM::action(JsonObject doc) {
  set_pwm(doc["value"]);
}

DynamicJsonDocument DevicePWM::sendData() {
  DynamicJsonDocument doc(500);
  return doc;
}

bool DevicePWM::is_output() {
  return true;
}

void DevicePWM::print() {
  Serial.println(F("PWM: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" pin:")); Serial.print(pin); Serial.print(F(" value:")); Serial.print(value); Serial.println(F("%"));
}

uint8_t DevicePWM::convertPercentToPWM(uint8_t percent) { 
  uint8_t tmp_val = round((255/100.0)*percent);
  if (tmp_val > 255) {
    return 255;
  } else if (tmp_val < 0) {
    return 0;
  } else {
    return tmp_val;
  }
}