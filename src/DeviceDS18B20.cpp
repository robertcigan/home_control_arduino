#include "DeviceDS18B20.h"

DeviceDS18B20::DeviceDS18B20(uint32_t device_id, uint8_t pin, uint32_t poll) {
  setup();
  this->pin = pin;
  this->device_id = device_id;
  this->poll = poll;
  pinMode(pin, INPUT);
  this->one_wire = new OneWire(pin);
  this->sensor = new DS18B20(this->one_wire);
  this->sensor->begin();
  this->sensor->setResolution(10);
  this->temperature_asked = false;
  #if defined(WITH_SERIAL)
    print();
  #endif
}

void DeviceDS18B20::loop() {
  if ((millis() - last_run) >= poll) { // read value if over the poll time or millis rotated
    ask_for_temperature();
    last_run = millis();
  }
  if (temperature_asked && sensor->isConversionComplete()) {
    float new_value = sensor->getTempC();
    if (new_value != value) {
      last_value_change = millis();
      value = new_value;
      #if defined(SHOW_VALUES_IN_SERIAL)
        Serial.print(F("DS18B20 pin: ")); Serial.print(pin); Serial.print(F(" change detected to ")); Serial.println(value, 2);
      #endif
      report = true;
    }
    temperature_asked = false;
    value_initialized = true;
  }
}

void DeviceDS18B20::ask_for_temperature() {
  sensor->requestTemperatures();
  temperature_asked = true;
}

void DeviceDS18B20::action(JsonObject doc) {
}

DynamicJsonDocument DeviceDS18B20::sendData() {
  DynamicJsonDocument doc(500);
  JsonObject json_object = doc.createNestedObject("device");
  json_object["value"] = value;
  json_object["id"] = device_id;
  return doc;
}

void DeviceDS18B20::uninitialize() {
  delete sensor;
  delete one_wire;
  sensor = NULL;
  one_wire = NULL;
}

bool DeviceDS18B20::is_output() {
  return false;
}

#if defined(WITH_SERIAL)
  void DeviceDS18B20::print() {
    Serial.println(F("DS18B20: ")); Serial.print(F(" id:")); Serial.println(device_id); Serial.print(F(" pin:")); Serial.println(pin); 
  }
#endif