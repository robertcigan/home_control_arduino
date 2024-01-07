#ifndef HOME_CONTROL_H
#define HOME_CONTROL_H

#if defined(__AVR_ATmega2560__)
  #define WITH_SERIAL
  #define WITH_SERIAL_CONFIG
  #define WITH_LED
#endif

#if defined(ARDUINO_ESP8266_ESP01)
  #define WITH_WIFI
  #define WITH_SERIAL
  #define WITH_SERIAL_CONFIG
  //#define WITH_FILE_CONFIG
#endif

#if defined(ARDUINO_ESP8266_NODEMCU)
  #define WITH_WIFI
  #define WITH_SERIAL
  #define WITH_SERIAL_CONFIG
  #define WITH_LED
#endif

#if defined(ESP32)
  #define WITH_WIFI
  #define WITH_SERIAL
  #define WITH_SERIAL_CONFIG
  #define WITH_LED
#endif

#if defined(WITH_SERIAL)
  //#define SHOW_MEMORY_IN_SERIAL
  //#define SHOW_VALUES_IN_SERIAL
#endif

#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(__AVR_ATmega2560__)
  #include <Ethernet.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif
#include <SimpleTimer.h>
#include <EEPROM.h>
#include <string.h>
#include <stdlib.h>

#include "Device.h"
#include "DeviceSwitch.h" 
#include "DeviceButton.h"
#include "DeviceRelay.h"
#include "DevicePlayer.h"
#include "DeviceDistance.h"
#include "DeviceAnalogInput.h"
#include "DeviceDS18B20.h"
#include "DevicePWM.h"
#include "DeviceCurtain.h"

#define MAX_DEVICES                     40
#define INPUT_BUFFER_SIZE               500
#define SERIAL_INPUT_BUFFER_SIZE        50
#define CONNECTION_TIMEOUT              30*1000L
#define VERSION                         9

#define EEPROM_INITIALIZED_VALUE        255
#define EEPROM_CONFIG_SET_OFFSET        0
#define EEPROM_CLIENT_IP_OFFSET         10
#define EEPROM_SERVER_IP_OFFSET         14
#define EEPROM_MAC_OFFSET               18

#if defined(WITH_WIFI)
  #define EEPROM_WIFI_SSID_OFFSET            24
  #define EEPROM_WIFI_PASS_OFFSET            64
  #define EEPROM_GATEWAY_OFFSET              104
#endif

class HomeControl {
  public:
    HomeControl();
    int device_count;
    void addDevice(Device &device);
    void deleteAllDevices();

    bool setup();
    void loop();
    static void availableMemory();
    uint32_t last_request;
    bool connectionExpired();
    bool devicesSet;

  private:
    byte mac[6];
    IPAddress server_ip;
    IPAddress client_ip;
    int port;

    char inData[INPUT_BUFFER_SIZE]; // Allocate some space for the incoming command string
    uint16_t inIndex;
    uint8_t inStatus; // 0 - wait, 1 - command
    SimpleTimer timer;
    #if defined(__AVR_ATmega2560__)
      EthernetClient client;
    #endif
    #if defined(WITH_WIFI)
      WiFiClient client;
      char wifi_ssid[20] = {'\0'};
      char wifi_pass[20] = {'\0'};
      IPAddress gateway_ip;
      float getRSSI();
    #endif
    bool setupConnection();
    void setNetwork();  
    Device *devices[MAX_DEVICES];

    void connect();
    bool loadConfiguration();
    bool saveConfiguration();
    #if defined(WITH_SERIAL)
      void printConfiguration();
    #endif
    void readInput();
    void resetInputData();
    void parseCommand();
    void pong();
    void sendDevices();
    void loopDevices();
    void reportDevices();

    #if defined(WITH_LED)
      void turnOnTestModeLED(int timeout);
      static void turnOffTestModeLED();
    #endif

    #if defined(WITH_SERIAL_CONFIG)
      uint16_t serialInIndex;
      char serialInData[SERIAL_INPUT_BUFFER_SIZE]; // Allocate some space for the incoming command string
      void readSerialInput();
      void resetSerialInputData();
      void parseSerialCommand();
    #endif
 };
#endif