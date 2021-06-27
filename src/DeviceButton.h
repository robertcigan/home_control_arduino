#ifndef DEVICE_BUTTON_H
#define DEVICE_BUTTON_H
#include <Arduino.h>
#include "Device.h"

class DeviceButton : public Device {
  public:
    DeviceButton(uint32_t device_id, uint8_t pin, uint32_t poll, bool inverted);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    void print();
  private:
    bool value;
    bool inverted;
};

#endif
