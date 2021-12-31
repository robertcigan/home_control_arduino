#ifndef DEVICE_H
#define DEVICE_H
#include <Arduino.h>
// #if defined(__AVR_ATmega2560__)
//   #include <Ethernet.h>
// #elif defined(ESP8266)
//   #include <ESP8266WiFi.h>
//   #include <ESP8266WiFiMulti.h>
// #elif defined(ESP32)
//   #include <WiFi.h>
// #endif
#include <ArduinoJson.h>

class Device {
  public:
    Device();
    ~Device();
    bool report;
    bool value_initialized;
    uint32_t device_id;
    virtual bool is_output();
    void setup();
    virtual void loop() = 0;
    virtual void action(JsonObject doc) = 0;
    virtual DynamicJsonDocument sendData() = 0;
    virtual void uninitialize() = 0;
    virtual void print() = 0;
    
  protected:
    uint8_t pin;
    uint8_t apin;
    uint32_t poll;
    uint32_t last_run;
    uint32_t last_value_change;
    void convertToAnalogPin();
    static float ADConversion(int input);
    static int PercentualPWMConversion(int input);
    static int PercentualServoConversion(int input);
};
#endif