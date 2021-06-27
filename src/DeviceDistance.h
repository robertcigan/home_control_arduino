#ifndef DEVICE_DISTANCE_H
#define DEVICE_DISTANCE_H
#include <Arduino.h>
#include "Device.h"

class DeviceDistance : public Device {
  public:
    DeviceDistance(uint32_t device_id, uint8_t write_pin, uint8_t read_pin, uint32_t poll);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    void print();
  private:
    uint8_t read_pin;
    uint8_t write_pin;
    uint32_t value;
    uint32_t read_distance();
};

#endif
