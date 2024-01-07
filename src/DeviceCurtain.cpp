#include "DeviceCurtain.h"

DeviceCurtain::DeviceCurtain(uint32_t device_id, uint8_t open_pin, uint32_t close_pin, bool inverted) {
  setup();
  this->open_pin = open_pin;
  this->close_pin = close_pin;
  this->inverted = inverted;
  this->device_id = device_id;
  this->poll = 0;
  this->action_running = false;
  pinMode(open_pin, OUTPUT);
  pinMode(close_pin, OUTPUT);
  digitalWrite(open_pin, inverted ? LOW : HIGH);
  digitalWrite(close_pin, inverted ? LOW : HIGH);
  #if defined(WITH_SERIAL)
    print();
  #endif
}

void DeviceCurtain::loop() {
  if (action_running && (millis() - action_started) >= action_length) {
    stop_curtain();
  }
}

void DeviceCurtain::open_curtain(uint32_t action_time) {
  if (action_time > 100 && action_time < 18e4) {
    #if defined(SHOW_VALUES_IN_SERIAL)
      Serial.print(F("Curtain pin: ")); Serial.print(open_pin); Serial.print(F(" & ")); Serial.print(close_pin); Serial.print(F(" opening started for "); Serial.print(action_time); Serial.println(F("ms."));
    #endif
    digitalWrite(open_pin, inverted ? HIGH : LOW);
    action_length = action_time;
    action_started = millis();
    action_running = true;
  }
}

void DeviceCurtain::close_curtain(uint32_t action_time) {
  if (action_time > 0 && action_time <= 18e4) {
    #if defined(SHOW_VALUES_IN_SERIAL)
      Serial.print(F("Curtain pin: ")); Serial.print(open_pin); Serial.print(F(" & ")); Serial.print(close_pin); Serial.print(F(" opening started for "); Serial.print(action_time); Serial.println(F("ms."));
    #endif
    digitalWrite(close_pin, inverted ? HIGH : LOW);
    action_length = action_time;
    action_started = millis();
    action_running = true;
  }
}

void DeviceCurtain::stop_curtain() {
  #if defined(SHOW_VALUES_IN_SERIAL)
    Serial.print(F("Curtain pin: ")); Serial.print(open_pin); Serial.print(F(" & ")); Serial.print(close_pin);Serial.println(F(" action stopped"));
  #endif
  action_running = false;
  digitalWrite(open_pin, inverted ? LOW : HIGH);
  digitalWrite(close_pin, inverted ? LOW : HIGH);
}

void DeviceCurtain::action(JsonObject doc) {
  if (action_running) {
    stop_curtain();
  }
  if (doc["action"] == "open") {
    open_curtain(doc["value"]);
  } else if (doc["action"] == "close") {
    close_curtain(doc["value"]);
  }
}

DynamicJsonDocument DeviceCurtain::sendData() {
  DynamicJsonDocument doc(500);
  return doc;
}

void DeviceCurtain::uninitialize() {
}

bool DeviceCurtain::is_output() {
  return true;
}

#if defined(WITH_SERIAL)
  void DeviceCurtain::print() {
    Serial.println(F("DS18B20: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" pin:")); Serial.println(pin); 
  }
#endif