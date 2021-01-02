#include "DeviceRelay.h"

DeviceRelay::DeviceRelay(uint32_t device_id, uint8_t pin) {
  setup();
  this->pin = pin;
  this->device_id = device_id;
  this->value = HIGH;
  // initialize output
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  print();
}

void DeviceRelay::loop() {
}

void DeviceRelay::set_relay(bool new_value) {
  if (new_value != value) {
    last_value_change = millis();
    value = new_value;
    digitalWrite(pin, value ? LOW : HIGH);
    Serial.print(F("Pin: ")); Serial.print(pin); Serial.print(F(" relay set to ")); Serial.println(value);
  } 
}

void DeviceRelay::uninitialize() { 
  digitalWrite(pin, HIGH);
}

void DeviceRelay::action(JsonObject doc) {
  set_relay(doc["value"]);
}

DynamicJsonDocument DeviceRelay::sendData() {
  DynamicJsonDocument doc(500);
  return doc;
}

bool DeviceRelay::is_output() {
  return true;
}

void DeviceRelay::print() {
  Serial.println(F("Relay: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" pin:")); Serial.print(pin); Serial.print(F(" value:")); Serial.println(value);
}
