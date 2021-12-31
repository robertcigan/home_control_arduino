#include "DeviceRelay.h"

DeviceRelay::DeviceRelay(uint32_t device_id, uint8_t pin, bool default_value) {
  setup();
  this->pin = pin;
  this->device_id = device_id;
  this->value = default_value;
  // initialize output
  pinMode(pin, OUTPUT);
  digitalWrite(pin, this->value ? LOW : HIGH);
  print();
}

void DeviceRelay::loop() {
}

void DeviceRelay::set_relay(bool new_value) {
  if (new_value != value) {
    last_value_change = millis();
    value = new_value;
    digitalWrite(pin, value ? LOW : HIGH);
    #if defined(SHOW_VALUES_IN_SERIAL)
      Serial.print(F("Pin: ")); Serial.print(pin); Serial.print(F(" relay set to ")); Serial.println(value);
    #endif
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
