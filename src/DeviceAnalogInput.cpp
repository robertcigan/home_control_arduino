#include "DeviceAnalogInput.h"

DeviceAnalogInput::DeviceAnalogInput(uint32_t device_id, uint8_t apin) {
  setup();
  this->apin = apin;
  this->device_id = device_id;
  this->poll = 5000;
  pinMode(apin, INPUT);
  print();
}

void DeviceAnalogInput::loop() {
  if ((last_run + poll) < millis() || last_run > millis()) { // read value if over the poll time or millis rotated
    float new_value = analogRead(apin) / 1024.0;
    if (new_value != value) {
      last_value_change = millis();
      value = new_value;
      Serial.print(F("Value: ")); Serial.print(apin); Serial.print(F(" change detected to ")); Serial.println(value);
      report = true;
    }
    value_initialized = true;
    last_run = millis();
  }
}

void DeviceAnalogInput::action(JsonObject doc) {
}

DynamicJsonDocument DeviceAnalogInput::sendData() {
  DynamicJsonDocument doc(500);
  JsonObject json_object = doc.createNestedObject("device");
  json_object["value"] = value;
  json_object["id"] = device_id;
  return doc;
}

void DeviceAnalogInput::uninitialize() {
  pinMode(apin, INPUT);
}

bool DeviceAnalogInput::is_output() {
  return false;
}

void DeviceAnalogInput::print() {
  Serial.println(F("Value: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" analog pin:")); Serial.print(apin); 
}