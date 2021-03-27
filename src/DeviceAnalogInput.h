#ifndef DEVICE_ANALOG_INPUT_H
#define DEVICE_ANALOG_INPUT_H
#include <Arduino.h>
#include "Device.h"

class DeviceAnalogInput : public Device {
  public:
    DeviceAnalogInput(uint32_t device_id, uint8_t apin);
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
