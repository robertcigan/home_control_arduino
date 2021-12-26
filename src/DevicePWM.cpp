#include "DevicePWM.h"

DevicePWM::DevicePWM(uint32_t device_id, uint8_t pin) {
  setup();
  this->pin = pin;
  this->device_id = device_id;
  this->value = HIGH;
  // initialize output
  pinMode(pin, OUTPUT);
  analogWrite(pin, 0);
  print();
}

void DevicePWM::loop() {
}

void DevicePWM::set_pwm(uint8_t new_value) {
  if (new_value != value) {
    last_value_change = millis();
    value = new_value;
    analogWrite(pin, value);
    #if defined(SHOW_VALUES_IN_SERIAL)
      Serial.print(F("Pin: ")); Serial.print(pin); Serial.print(F(" PWM set to ")); Serial.println(value);
    #endif
  } 
}

void DevicePWM::uninitialize() { 
  analogWrite(pin, 0);
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
  Serial.println(F("PWM: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" pin:")); Serial.print(pin); Serial.print(F(" value:")); Serial.println(value);
}
