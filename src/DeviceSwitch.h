#ifndef DEVICE_SWITCH_H
#define DEVICE_SWITCH_H
#include <Arduino.h>
#include "Device.h"

class DeviceSwitch : public Device {
  public:
    DeviceSwitch(uint32_t device_id, uint8_t pin);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    void print();
  private:
    bool value;
};

#endif