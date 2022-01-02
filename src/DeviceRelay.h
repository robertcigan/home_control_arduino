#ifndef DEVICE_RELAY_H
#define DEVICE_RELAY_H
#include <Arduino.h>
#include "Device.h"

class DeviceRelay : public Device {
  public:
    DeviceRelay(uint32_t device_id, uint8_t pin, bool default_value);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    #if defined(WITH_SERIAL)
      void print();
    #endif
    
    void set_relay(bool new_value);
  private:
    bool value;
};

#endif
