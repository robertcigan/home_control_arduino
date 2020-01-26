#ifndef HOME_CONTROL_H
#define HOME_CONTROL_H
#include <Arduino.h>

#include <ArduinoJson.h>
#include <Ethernet.h>
#include <SD.h>
#include "src/SimpleTimer/SimpleTimer.h"

#include "Device.h"
#include "DeviceSwitch.h"
#include "DeviceButton.h"
#include "DeviceRelay.h"
#include "DevicePlayer.h"

//#define DEVICE_UDP_PORT 5001
//#define AUTOMATOR_UDP_PORT 5001
#define MAX_DEVICES 40
#define INPUT_BUFFER_SIZE 500
#define VERSION 1

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
    int production;
    byte mac[6];
    IPAddress ip;
    uint8_t server[4];
    uint8_t server_production[4];
    uint8_t server_development[4];
    uint8_t ip_offset;
    int port;
    char inData[INPUT_BUFFER_SIZE]; // Allocate some space for the incoming command string
    char inChar=-1; 
    byte inIndex;
    int inStatus; // 0 - wait, 1 - command
    SimpleTimer timer;
    EthernetClient client;
    Device *devices[MAX_DEVICES];

    bool setupEthernet();

    void connect();
    bool loadConfiguration();
    void printConfiguration();
    static void printDirectory(File dir, int num_tabs);
    void readInput();
    void resetInputData();
    bool parseCommand();
    void pong();

    void turnOnTestModeLED(int timeout);
    static void turnOffTestModeLED();
 };

#endif
