#ifndef DEVICE_PLAYER_H
#define DEVICE_PLAYER_H
#include <Arduino.h>
#include <RedMP3.h>
#include "Device.h"

class DevicePlayer : public Device {
  public:
    DevicePlayer(uint32_t device_id);
    bool is_output();
    void loop();
    void action(JsonObject doc);
    DynamicJsonDocument sendData();
    void uninitialize();
    #if defined(WITH_SERIAL)
      void print();
    #endif
    void play(uint8_t directory, uint8_t file);
    void volume(uint8_t vol);
  private:
    MP3 *player;
    
};

#endif
