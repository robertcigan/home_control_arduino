#include "DeviceSwitch.h"

DeviceSwitch::DeviceSwitch(uint32_t device_id, uint8_t pin, uint32_t poll, bool inverted) {
  setup();
  this->pin = pin;
  this->device_id = device_id;
  this->poll = poll;
  this->inverted = inverted;
  if (inverted) {
    pinMode(pin, INPUT);
  } else {
    pinMode(pin, INPUT_PULLUP);
  }
  #if defined(WITH_SERIAL)
    print();
  #endif
}

void DeviceSwitch::loop() {
  if ((last_run + poll) < millis() || last_run > millis()) { // read value if over the poll time or millis rotated
    bool new_value;
    if (inverted) {
      new_value = digitalRead(pin);
    } else {
      new_value = !digitalRead(pin); // PULLUP - ground pin - connected == 1
    }
    if (new_value != value) {
      last_value_change = millis();
      value = new_value;
      #if defined(SHOW_VALUES_IN_SERIAL)
        Serial.print(F("Pin: ")); Serial.print(pin); Serial.print(F(" change detected to ")); Serial.println(value);
      #endif
      report = true;
    }
    value_initialized = true;
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

#if defined(WITH_SERIAL)
  void DeviceSwitch::print() {
    Serial.println(F("Switch: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" pin:")); Serial.println(pin);
  }
#endif