#ifndef DEVICE_ANALOG_INPUT_H
#define DEVICE_ANALOG_INPUT_H
#include <Arduino.h>
#include "Device.h"

class DeviceAnalogInput : public Device {
  public:
    DeviceAnalogInput(uint32_t device_id, uint8_t apin, uint32_t poll);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    #if defined(WITH_SERIAL)
      void print();
    #endif
  private:
    float value;
};

#endif
