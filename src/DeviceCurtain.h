#ifndef DEVICE_CURTAIN_H
#define DEVICE_CURTAIN_H
#include <Arduino.h>
#include "Device.h"

class DeviceCurtain : public Device {
  public:
    DeviceCurtain(uint32_t device_id, uint8_t open_pin, uint32_t close_pin);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    #if defined(WITH_SERIAL)
      void print();
    #endif
  private:
    uint8_t open_pin;
    uint8_t close_pin;
    uint32_t action_length;
    uint32_t action_started;
    bool action_running;
    void open_curtain(uint32_t action_time);
    void close_curtain(uint32_t action_time);
    void stop_curtain();
};

#endif