#ifndef DEVICE_DS18B20_H
#define DEVICE_DS18B20_H
#include <Arduino.h>
#include <OneWire.h>
#include <DS18B20.h>
#include "Device.h"

class DeviceDS18B20 : public Device {
  public:
    DeviceDS18B20(uint32_t device_id, uint8_t pin, uint32_t poll);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    void print();
  private:
    float value;
    OneWire *one_wire;
    DS18B20 *sensor;
    void ask_for_temperature();
    bool temperature_asked;
};

#endif