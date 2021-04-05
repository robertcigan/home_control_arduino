#include "DeviceDistance.h"

DeviceDistance::DeviceDistance(uint32_t device_id, uint8_t write_pin, uint8_t read_pin) {
  setup();
  this->read_pin = read_pin;
  this->write_pin = write_pin;
  this->device_id = device_id;
  this->poll = 30000;
  pinMode(write_pin, OUTPUT);
  pinMode(read_pin, INPUT);
  print();
}

void DeviceDistance::loop() {
  if ((last_run + poll) < millis() || last_run > millis()) { // read value if over the poll time or millis rotated
    uint32_t new_value = read_distance();
    if (new_value != value) {
      last_value_change = millis();
      value = new_value;
      #if defined(SHOW_VALUES_IN_SERIAL)
        Serial.print(F("Distance: read pin ")); Serial.print(read_pin); Serial.print(F(" change detected to ")); Serial.println(value);
      #endif
      report = true;
    }
    value_initialized = true;
    last_run = millis();
  }
}

uint32_t DeviceDistance::read_distance() {
  digitalWrite(write_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(write_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(write_pin, LOW);
  uint32_t duration = pulseIn(read_pin, HIGH);
  uint32_t distance = duration*0.034/2;
  if (distance > 999) {
    distance = 999;
  }
  return distance;
}

void DeviceDistance::action(JsonObject doc) {
}

DynamicJsonDocument DeviceDistance::sendData() {
  DynamicJsonDocument doc(500);
  JsonObject json_object = doc.createNestedObject("device");
  json_object["value"] = value;
  json_object["id"] = device_id;
  return doc;
}

void DeviceDistance::uninitialize() {
  pinMode(write_pin, INPUT);
  pinMode(read_pin, INPUT);
}

bool DeviceDistance::is_output() {
  return false;
}

void DeviceDistance::print() {
  Serial.println(F("Distance: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" write pin:")); Serial.print(write_pin); Serial.print(F(" read pin:")); Serial.println(read_pin);
}
