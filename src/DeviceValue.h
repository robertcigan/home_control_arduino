#ifndef DEVICE_VALUE_H
#define DEVICE_VALUE_H
#include <Arduino.h>
#include "Device.h"

class DeviceValue : public Device {
  public:
    DeviceValue(uint32_t device_id, uint8_t apin);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    void print();
  private:
    float value;
};

#endif
