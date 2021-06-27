#include "DeviceButton.h"

DeviceButton::DeviceButton(uint32_t device_id, uint8_t pin, uint32_t poll, bool inverted) {
  setup();
  this->pin = pin;
  this->device_id = device_id;
  this->poll = poll;
  this->inverted = inverted;
  pinMode(pin, INPUT_PULLUP);
  print();
}

void DeviceButton::loop() {
  if ((last_run + poll) < millis() || last_run > millis()) { // read value if over the poll time or millis rotated
    bool new_value = !digitalRead(pin); // PULLUP - ground pin - connected == 1
    if (new_value != value) {
      last_value_change = millis();
      if (new_value == HIGH) {
        #if defined(SHOW_VALUES_IN_SERIAL)
          Serial.print(F("Pin: ")); Serial.print(pin); Serial.println(F(" change detected"));
        #endif
        report = true;
      }
      value = new_value;      
    }
    value_initialized = true;
    last_run = millis();
  }
}

void DeviceButton::action(JsonObject doc) {
}

DynamicJsonDocument DeviceButton::sendData() {
  DynamicJsonDocument doc(500);
  JsonObject json_object = doc.createNestedObject("device");
  json_object["id"] = device_id;
  return doc;
}

void DeviceButton::uninitialize() {
}

bool DeviceButton::is_output() {
  return false;
}

void DeviceButton::print() {
  Serial.println(F("Button: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" pin:")); Serial.println(pin);
}
