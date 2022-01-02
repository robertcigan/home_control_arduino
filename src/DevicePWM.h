#ifndef DEVICE_PWM_H
#define DEVICE_PWM_H
#include <Arduino.h>
#include "Device.h"

class DevicePWM : public Device {
  public:
    DevicePWM(uint32_t device_id, uint8_t pin, uint8_t default_value);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    #if defined(WITH_SERIAL)
      void print();
    #endif
    void set_pwm(uint8_t new_value);
  private:
    uint8_t value;
    uint8_t convertPercentToPWM(uint8_t percent);
};

#endif
