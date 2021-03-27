#ifndef HOME_CONTROL_H
#define HOME_CONTROL_H
#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(__AVR__)
  #include <Ethernet.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WiFiMulti.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiMulti.h>
#endif
//#include <SD.h>
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

#define MAX_DEVICES                     40
#define INPUT_BUFFER_SIZE               500
#define SERIAL_INPUT_BUFFER_SIZE        50
#define VERSION                         2

#define EEPROM_INITIALIZED_VALUE         255
#define EEPROM_CONFIG_SET_OFFSET        0
#define EEPROM_CLIENT_IP_OFFSET         10
#define EEPROM_SERVER_IP_OFFSET         14
#define EEPROM_MAC_OFFSET               18

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
    uint16_t autoreset; 
    bool connectionExpired();

  private:
    byte mac[6];
    IPAddress server_ip;
    IPAddress client_ip;
    int port;

    char inData[INPUT_BUFFER_SIZE]; // Allocate some space for the incoming command string
    uint16_t inIndex;
    uint8_t inStatus; // 0 - wait, 1 - command
    SimpleTimer timer;
    #if defined(__AVR__)
      EthernetClient client;
    #elif defined(ESP8266)
      ESP8266WiFiMulti wifiMulti;
      WiFiClient client;
    #elif defined(ESP32)
      WiFiMulti wifiMulti;
      WiFiClient client;
    #endif
    bool setupNetwork();    
    Device *devices[MAX_DEVICES];

    void connect();
    bool loadConfiguration();
    bool saveConfiguration();
    void printConfiguration();
    void readInput();
    void resetInputData();
    void parseCommand();
    void pong();

    void turnOnTestModeLED(int timeout);
    static void turnOffTestModeLED();

    uint16_t serialInIndex;
    char serialInData[SERIAL_INPUT_BUFFER_SIZE]; // Allocate some space for the incoming command string
    void readSerialInput();
    void resetSerialInputData();
    void parseSerialCommand();
 };

#endif
