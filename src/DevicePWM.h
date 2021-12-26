#ifndef DEVICE_PWM_H
#define DEVICE_PWM_H
#include <Arduino.h>
#include "Device.h"

class DevicePWM : public Device {
  public:
    DevicePWM(uint32_t device_id, uint8_t pin);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    void print();
    
    void set_pwm(uint8_t new_value);
  private:
    uint8_t value;
};

#endif
