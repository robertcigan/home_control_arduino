#include "DeviceAnalogInput.h"

DeviceAnalogInput::DeviceAnalogInput(uint32_t device_id, uint8_t apin, uint32_t poll) {
  setup();
  this->apin = apin;
  this->device_id = device_id;
  this->poll = 5000;
  pinMode(apin, INPUT);
  #if defined(WITH_SERIAL)
    print();
  #endif
}

void DeviceAnalogInput::loop() {
  if ((millis() - last_run) >= poll) { // read value if over the poll time or millis rotated
    
    #if defined(__AVR_ATmega2560__)
      float conversion =  5.0 / 1023;
    #elif defined(__XTENSA__)
      float conversion =  3.3 / 1023;
    #endif
    float new_value = (int)(analogRead(apin) * conversion * 100 + 0.5) / 100.0;
    if (new_value != value) {
      last_value_change = millis();
      value = new_value;
      #if defined(SHOW_VALUES_IN_SERIAL)
        Serial.print(F("Analog pin: ")); Serial.print(apin); Serial.print(F(" change detected to ")); Serial.println(value);
      #endif
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

#if defined(WITH_SERIAL)
  void DeviceAnalogInput::print() {
    Serial.println(F("Value: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" analog pin:")); Serial.println(apin); 
  }
#endif