#include "DeviceSwitch.h"

DeviceSwitch::DeviceSwitch(uint32_t device_id, uint8_t pin) {
  setup();
  this->pin = pin;
  this->device_id = device_id;
  this->poll = 250;
  pinMode(pin, INPUT_PULLUP);
  print();
}

void DeviceSwitch::loop() {
  if ((last_run + poll) < millis() || last_run > millis()) { // read value if over the poll time or millis rotated
    bool new_value = !digitalRead(pin);
    if (new_value != value) {
      last_value_change = millis();
      value = new_value;
      Serial.print(F("Pin: ")); Serial.print(pin); Serial.print(F(" change detected to ")); Serial.println(value);
      report = true;
    } 
    last_run = millis();
  }
}
void DeviceSwitch::action(JsonObject doc) {
}

DynamicJsonDocument DeviceSwitch::sendData() {
  DynamicJsonDocument doc(500);
  JsonObject json_object = doc.createNestedObject("device");
  json_object["value"] = value;
  json_object["id"] = device_id;
  return doc;
}

void DeviceSwitch::uninitialize() {
}

bool DeviceSwitch::is_output() {
  return false;
}

void DeviceSwitch::print() {
  Serial.println(F("Switch: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" pin:")); Serial.println(pin);
}
