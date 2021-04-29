#ifndef HOME_CONTROL_H
#define HOME_CONTROL_H
#define SHOW_MEMORY_IN_SERIAL
#define SHOW_VALUES_IN_SERIAL
#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(__AVR_ATmega2560__)
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
#include "DeviceDS18B20.h"

#define MAX_DEVICES                     40
#define INPUT_BUFFER_SIZE               500
#define SERIAL_INPUT_BUFFER_SIZE        50
#define VERSION                         3

#define EEPROM_INITIALIZED_VALUE        255
#define EEPROM_CONFIG_SET_OFFSET        0
#define EEPROM_CLIENT_IP_OFFSET         10
#define EEPROM_SERVER_IP_OFFSET         14
#define EEPROM_MAC_OFFSET               18

#if defined(ESP8266) || defined(ESP32)
  #define EEPROM_WIFI_SSID_1_OFFSET            24
  #define EEPROM_WIFI_SSID_2_OFFSET            44
  #define EEPROM_WIFI_PASS_1_OFFSET            64
  #define EEPROM_WIFI_PASS_2_OFFSET            84
  #define EEPROM_GATEWAY_OFFSET                104
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
    uint16_t autoreset; 
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
    #elif defined(ESP8266)
      ESP8266WiFiMulti wifiMulti;
      WiFiClient client;
      char wifi_ssid_1[20] = {'\0'};
      char wifi_ssid_2[20] = {'\0'};
      char wifi_pass_1[20] = {'\0'};
      char wifi_pass_2[20] = {'\0'};
      IPAddress gateway_ip;
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
    void sendDevices();

    void turnOnTestModeLED(int timeout);
    static void turnOffTestModeLED();

    uint16_t serialInIndex;
    char serialInData[SERIAL_INPUT_BUFFER_SIZE]; // Allocate some space for the incoming command string
    void readSerialInput();
    void resetSerialInputData();
    void parseSerialCommand();
 };

#endif
