#include "HomeControl.h"

HomeControl::HomeControl() {
  this->devicesSet = false;
  this->device_count = 0;
  this->last_request = millis();
  this->mac[0] = 0xDE;
  this->mac[1] = 0xAA;
  this->mac[2] = 0xBB;
  this->mac[3] = 0xCC;
  this->mac[4] = 0xDE;
  this->mac[5] = 0x00;
  this->client_ip = IPAddress(192, 168, 0, 0);
  this->server_ip = IPAddress(192, 168, 0, 0);
  this->port = 7777;

  #if defined(WITH_WIFI)
    this->gateway_ip = IPAddress(192, 168, 0, 0);
  #endif
  
  this->inIndex = 0;
  this->inStatus = 0; // 0 - wait, 1 - command
  #if defined(WITH_SERIAL_CONFIG)
    this->serialInIndex = 0;
  #endif
}

bool HomeControl::setup() {
  #if defined(WITH_LED)
    pinMode(LED_BUILTIN, OUTPUT); //TEST LED
  #endif
  #if defined(__AVR_ATmega2560__)
    pinMode(10, OUTPUT);   // set the Ethernet SS pin as an output (necessary!)
    digitalWrite(10, HIGH);
  #endif
    
  #if defined(ESP8266) || defined(ESP32)
    #if defined(WITH_SERIAL_CONFIG)
      EEPROM.begin(512);
    #endif
  #endif
  
  if (loadConfiguration()) {
    #if defined(WITH_SERIAL)
      Serial.println(F("Configuration successfull!"));
    #endif
  } else {
    return false; 
  }
  if (setupConnection()) {
    #if defined(WITH_SERIAL)
      Serial.println(F("Network setup successfull!"));
    #endif
  } else {
    return false; 
  }
  availableMemory();
  return true;
}

bool HomeControl::loadConfiguration() {
  #if defined(WITH_FILE_CONFIG)
    // implement load config from file
  #elif defined(WITH_SERIAL_CONFIG)
    if (EEPROM.read(EEPROM_CONFIG_SET_OFFSET) == EEPROM_INITIALIZED_VALUE && 
      EEPROM.read(EEPROM_CONFIG_SET_OFFSET + 1) == EEPROM_INITIALIZED_VALUE) {
        
      for(int i = 0; i <= 3; i++) {
      client_ip[i] = EEPROM.read(i + EEPROM_CLIENT_IP_OFFSET);
      }
      for(int i = 0; i <= 3; i++) {
        server_ip[i] = EEPROM.read(i + EEPROM_SERVER_IP_OFFSET);
      }
      for(int i = 0; i <= 5; i++) {
        mac[i] = EEPROM.read(i + EEPROM_MAC_OFFSET);
      }
      
      #if defined(WITH_WIFI)
        for(int i = 0; i <= 19; i++) {
          wifi_ssid[i] = EEPROM.read(i + EEPROM_WIFI_SSID_OFFSET);
          wifi_pass[i] = EEPROM.read(i + EEPROM_WIFI_PASS_OFFSET);
        }
        for(int i = 0; i <= 3; i++) {
          gateway_ip[i] = EEPROM.read(i + EEPROM_GATEWAY_OFFSET);
        }
      #endif
    } else {
      Serial.println("EEPROM not initialized, storing default values now.");
      #if defined(__AVR_ATmega2560__)
        EEPROM.update(EEPROM_CONFIG_SET_OFFSET, EEPROM_INITIALIZED_VALUE);
        EEPROM.update(EEPROM_CONFIG_SET_OFFSET + 1, EEPROM_INITIALIZED_VALUE);
      #elif defined(ESP8266) || defined(ESP32)
        EEPROM.write(EEPROM_CONFIG_SET_OFFSET, EEPROM_INITIALIZED_VALUE);
        EEPROM.write(EEPROM_CONFIG_SET_OFFSET + 1, EEPROM_INITIALIZED_VALUE);
      #endif
      saveConfiguration();
    }
  #endif
  #if defined(WITH_SERIAL)
    printConfiguration();
  #endif
  return true;
}

bool HomeControl::saveConfiguration() {
  #if defined(WITH_SERIAL_CONFIG)
    #if defined(__AVR_ATmega2560__)
      for(int i = 0; i <= 3; i++) {
        EEPROM.update(i + EEPROM_CLIENT_IP_OFFSET, client_ip[i]);
      }
      for(int i = 0; i <= 3; i++) {
        EEPROM.update(i + EEPROM_SERVER_IP_OFFSET, server_ip[i]);
      }
      for(int i = 0; i <= 5; i++) {
        EEPROM.update(i + EEPROM_MAC_OFFSET, mac[i]);
      }
    #elif defined(WITH_WIFI)
      for(int i = 0; i <= 3; i++) {
        EEPROM.write(i + EEPROM_CLIENT_IP_OFFSET, client_ip[i]);
      }
      for(int i = 0; i <= 3; i++) {
        EEPROM.write(i + EEPROM_SERVER_IP_OFFSET, server_ip[i]);
      }
      for(int i = 0; i <= 5; i++) {
        EEPROM.write(i + EEPROM_MAC_OFFSET, mac[i]);
      }
      for(int i = 0; i <= 19; i++) {
        EEPROM.write(i + EEPROM_WIFI_SSID_OFFSET, wifi_ssid[i]);
        EEPROM.write(i + EEPROM_WIFI_PASS_OFFSET, wifi_pass[i]);
      }
      for(int i = 0; i <= 3; i++) {
        EEPROM.write(i + EEPROM_GATEWAY_OFFSET, gateway_ip[i]);
      }

      if (EEPROM.commit()) {
        #if defined(WITH_SERIAL)
          Serial.println(F("EEPROM successfully committed"));
        #endif
      } else {
        #if defined(WITH_SERIAL)
          Serial.println(F("ERROR! EEPROM commit failed"));
        #endif
      }
    #endif
  #endif
  return true;
}

bool HomeControl::setupConnection() {
  #if defined(WITH_WIFI)
    #if defined(WITH_SERIAL) && defined(SHOW_VALUES_IN_SERIAL)
      Serial.setDebugOutput(true);
    #endif
    WiFi.persistent(false);
    WiFi.setAutoConnect(false);
    WiFi.mode(WIFI_STA);
    WiFi.config(client_ip, gateway_ip, gateway_ip);
    WiFi.begin(wifi_ssid, wifi_pass);
  #endif
  setNetwork();
  return true;
}

void HomeControl::setNetwork() {
  #if defined(__AVR_ATmega2560__)
    Ethernet.begin(mac, client_ip);
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      #if defined(WITH_SERIAL)
        Serial.println(F("Ethernet shield was not found."));
      #endif
      return false;
    } else if (Ethernet.hardwareStatus() == EthernetW5100) {
      #if defined(WITH_SERIAL)
        Serial.println(F("W5100 Ethernet controller detected."));
      #endif
    } else if (Ethernet.hardwareStatus() == EthernetW5200) {
      #if defined(WITH_SERIAL)
        Serial.println(F("W5200 Ethernet controller detected."));
      #endif
    } else if (Ethernet.hardwareStatus() == EthernetW5500) {
      #if defined(WITH_SERIAL)
        Serial.println(F("W5500 Ethernet controller detected."));
      #endif
    }
    if (Ethernet.linkStatus() == Unknown) {
      #if defined(WITH_SERIAL)
        Serial.println(F("Link status unknown. Link status detection is only available with W5200 and W5500."));
      #endif
    } else if (Ethernet.linkStatus() == LinkON) {
      #if defined(WITH_SERIAL)
        Serial.println(F("Link status: On"));
      #endif
    } else if (Ethernet.linkStatus() == LinkOFF) {
      #if defined(WITH_SERIAL)
        Serial.println(F("Link status: Off"));
      #endif
    }
  #endif
}

void HomeControl::connect() {
  setNetwork(); // set network again if W5500 looses data due to rest or so
  #if defined(__AVR_ATmega2560__)
    client.stop();
    #if defined(WITH_SERIAL)
      Serial.print(F("Connecting to server: "));
    #endif
    if (client.connect(server_ip, port)) {
      #if defined(WITH_SERIAL)
        Serial.println(F("OK"));
      #endif
      last_request = millis();
      if (!devicesSet) {
        sendDevices();
      }
    } else {
      #if defined(WITH_SERIAL)
        Serial.println(F("failed"));
      #endif
    }
  #elif defined(WITH_WIFI)
    client.stop();
    #if defined(WITH_SERIAL)
      Serial.print(F("Connecting to WiFi: "));
    #endif
    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - started) < 5000 ) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      #if defined(WITH_SERIAL)
        Serial.print(F("WiFi connected: "));
        Serial.println(WiFi.SSID());
        Serial.print(F("Signal Strength: "));
        Serial.print(getRSSI());
        Serial.println(F("%"));
        Serial.print(F("Connecting to server: "));
        Serial.print(F(" "));
      #endif
      if (client.connect(server_ip, port)) {
        #if defined(WITH_SERIAL)
          Serial.println(F("OK"));
        #endif
        last_request = millis();
        if (!devicesSet) {
          sendDevices();
        }
      } else {
        #if defined(WITH_SERIAL)
          Serial.println(F("failed"));
        #endif
      }
    } else {
      #if defined(WITH_SERIAL)
        Serial.println(F("WiFi not connected!"));
      #endif
    }
  #endif
}

void HomeControl::addDevice(Device &device) {
  if (device_count >= MAX_DEVICES) {
    #if defined(WITH_SERIAL)
      Serial.print(F("Maximum devices reached "));Serial.print(MAX_DEVICES);
    #endif
  } else {
    devices[device_count] = &device;
    device_count = device_count + 1;
    devicesSet = true;
  }
}

void HomeControl::deleteAllDevices() {
  for(int i = 0; i < device_count; i++) {
    if (devices[i]->is_output()) {
      devices[i]->uninitialize();
    }
    delete devices[i];
    devices[i] = NULL;
  }
  device_count = 0;
}

void HomeControl::readInput() {
  char inChar=-1;
  while(client.available() > 0) {
    if(inIndex < INPUT_BUFFER_SIZE) {
      inChar = client.read(); // Read a charact
      if (inChar == '\n') {
        inData[inIndex] = '\0';
        #if defined(WITH_SERIAL)
          Serial.print(F("Received:"));Serial.println(inData);
        #endif
        parseCommand();
        resetInputData();
        availableMemory();
        return;
      } else {
        inData[inIndex] = inChar; // Store it
        inIndex++; // Increment where to write next
      }
    } else {
      #if defined(WITH_SERIAL)
        Serial.println(F("Input data overflow!"));
      #endif
      resetInputData();
    }
  }
}

void HomeControl::resetInputData() {
  inIndex = 0;
  inStatus = 0;
  return;
}

void HomeControl::parseCommand() {
  last_request = millis();
  DynamicJsonDocument doc(INPUT_BUFFER_SIZE);
  DeserializationError err = deserializeJson(doc, inData);
  if (err) {
    #if defined(WITH_SERIAL)
      Serial.print(F("deserializeJson() failed with code ")); Serial.println(err.c_str());
    #endif
  } else {
    if (doc["add"]) {
      if (doc["add"]["type"] == F("switch")) {
        DeviceSwitch *device = new DeviceSwitch(doc["add"]["id"], doc["add"]["pin"], doc["add"]["poll"], doc["add"]["inverted"]);
        addDevice(*device);
      } else if (doc["add"]["type"] == F("button")) {
        DeviceButton *device = new DeviceButton(doc["add"]["id"], doc["add"]["pin"], doc["add"]["poll"], doc["add"]["inverted"]);
        addDevice(*device);
      } else if (doc["add"]["type"] == F("relay")) {
        DeviceRelay *device = new DeviceRelay(doc["add"]["id"], doc["add"]["pin"], doc["add"]["default"], doc["add"]["inverted"]);
        addDevice(*device);
      } else if (doc["add"]["type"] == F("player")) {
        DevicePlayer *device = new DevicePlayer(doc["add"]["id"].as<uint32_t>());
        addDevice(*device);
      } else if (doc["add"]["type"] == F("distance")) {
        DeviceDistance *device = new DeviceDistance(doc["add"]["id"], doc["add"]["write_pin"], doc["add"]["read_pin"], doc["add"]["poll"]);
        addDevice(*device);
      } else if (doc["add"]["type"] == F("analog_input")) {
        DeviceAnalogInput *device = new DeviceAnalogInput(doc["add"]["id"], doc["add"]["apin"], doc["add"]["poll"]);
        addDevice(*device);
      } else if (doc["add"]["type"] == F("ds18b20")) {
        DeviceDS18B20 *device = new DeviceDS18B20(doc["add"]["id"], doc["add"]["pin"], doc["add"]["poll"]);
        addDevice(*device);
      } else if (doc["add"]["type"] == F("pwm")) {
        DevicePWM *device = new DevicePWM(doc["add"]["id"], doc["add"]["pin"], doc["add"]["default"]);
        addDevice(*device);
      } else if (doc["add"]["type"] == F("curtain")) {
        DevicePWM *device = new DevicePWM(doc["add"]["id"], doc["add"]["open_pin"], doc["add"]["close_pin"]);
        addDevice(*device);
      }
    } else if (doc["reset_devices"]) {
      deleteAllDevices();
      devicesSet = false;
    } else if (doc["ping"]) {
      pong();
    } else if (doc["read"]) {
      for(int i = 0; i < device_count; i++) {
        if (!(devices[i]->is_output()) && devices[i]->device_id == doc["read"]["id"]) {
          devices[i]->report = true;
          break;
        }
      }
    } else if (doc["write"]) {
      for(int i = 0; i < device_count; i++) {
        if ((devices[i]->is_output()) && devices[i]->device_id == doc["write"]["id"]) {
          devices[i]->action(doc["write"]) ;
          break;
        }
      }
    } else {
      #if defined(WITH_SERIAL)
        Serial.println(F("Unknown command"));
      #endif
    }
  }
}

void HomeControl::pong() {
  DynamicJsonDocument doc(200);
  doc["pong"] = true;
  doc["version"] = VERSION;
  #if defined(WITH_WIFI)
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = getRSSI();
  #endif
  #if defined(WITH_SERIAL)
    Serial.print(F("Sending: "));
    serializeJson(doc, Serial);
    Serial.print("\n");
  #endif
  serializeJson(doc, client);
  client.write("\n");
  #if defined(WITH_LED)
    turnOnTestModeLED(250);
  #endif
}

void HomeControl::sendDevices() {
  client.write("{\"send_devices\": true}\n");
}

void HomeControl::loop() {
  #if defined(WITH_SERIAL_CONFIG)
    readSerialInput();
  #endif
  if (!client.connected() || connectionExpired()) {
    #if defined(WITH_LED)
      turnOnTestModeLED(0);
    #endif
    delay(5000);
    connect();
  } else {
    readInput();
    loopDevices();
    reportDevices();
    //flush all data from buffer to network
    client.flush();
  }
  timer.run();
}

void HomeControl::loopDevices() {
  // loop through all devices to read their data
  for(int i = 0; i < device_count; i++) {
    devices[i]->loop();
  }
}

void HomeControl::reportDevices() {
  // if there is value to report to server from devices, sent it
  for(int i = 0; i < device_count; i++) {
    if (devices[i]->report && devices[i]->value_initialized) {
      #if defined(WITH_SERIAL)
        Serial.print(F("Sending: "));
        serializeJson(devices[i]->sendData(), Serial);
        Serial.println();
      #endif
      serializeJson(devices[i]->sendData(), client);
      client.write("\n");
    }
    if (!(devices[i]->is_output())) {
      devices[i]->report = false;
    }
  }
  //flush all data from buffer to network
  client.flush();
}

/***** HELP FUNCTIONS ******/
#if defined(WITH_LED)
  void HomeControl::turnOnTestModeLED(int timeout) {
    #if defined(__AVR_ATmega2560__)
      digitalWrite(LED_BUILTIN, HIGH);
    #else
      digitalWrite(LED_BUILTIN, LOW);
    #endif
    if (timeout > 0) {
      timer.setTimeout(timeout, turnOffTestModeLED);
    }
  }

  void HomeControl::turnOffTestModeLED() {
    #if defined(__AVR_ATmega2560__)
      digitalWrite(LED_BUILTIN, LOW);
    #else
      digitalWrite(LED_BUILTIN, HIGH);
    #endif
  }
#endif

#if defined(WITH_SERIAL)
  void HomeControl::printConfiguration() {
    Serial.println(F("HomeControl "));
    Serial.print(F("Version: ")); Serial.println(VERSION);
    Serial.println(F("Network settings"));
    Serial.println(F("----------------"));
    Serial.print(F("MAC:"));
    for(int i = 0; i < 6; i++) {
      if (i != 5) {
        Serial.print(mac[i], HEX);
        Serial.print(":");
      } else {
        Serial.println(mac[i], HEX);
      }
    }
    Serial.print(F("Client IP:  ")); Serial.print(client_ip[0]); Serial.print(F(".")); Serial.print(client_ip[1]); Serial.print(F(".")); Serial.print(client_ip[2]); Serial.print(F(".")); Serial.println(client_ip[3]);
    Serial.print(F("Server IP:  ")); Serial.print(server_ip[0]); Serial.print("."); Serial.print(server_ip[1]); Serial.print(F(".")); Serial.print(server_ip[2]); Serial.print(F(".")); Serial.println(server_ip[3]);
    Serial.print(F("Port:       ")); Serial.println(port);
    #if defined(WITH_WIFI)
      Serial.print(F("Gateway IP:  ")); Serial.print(gateway_ip[0]); Serial.print("."); Serial.print(gateway_ip[1]); Serial.print(F(".")); Serial.print(gateway_ip[2]); Serial.print(F(".")); Serial.println(gateway_ip[3]);
      Serial.print(F("WiFi SSID:   ")); Serial.println(wifi_ssid);
      Serial.print(F("WiFi PASS:   ")); Serial.println(wifi_pass);
    #endif
    #if defined(WITH_SERIAL_CONFIG)
      Serial.println(F("Available commands:"));
      Serial.println(F("  server_ip=123.123.123.123"));
      Serial.println(F("  client_ip=123.123.123.123"));
      Serial.println(F("  mac=DE:AD:FF:FF:FF:FF"));
      #if defined(WITH_WIFI)
        Serial.println(F("  gateway_ip=123.123.123.123"));
        Serial.println(F("  ssid=some wifi name"));
        Serial.println(F("  pass=some wifi pass"));
      #endif
      Serial.println(F("  save"));
    #endif
  }
#endif


bool HomeControl::connectionExpired() {
  return (CONNECTION_TIMEOUT > 0 && (millis() - last_request) > CONNECTION_TIMEOUT);
}

void HomeControl::availableMemory() {
  #if defined(SHOW_MEMORY_IN_SERIAL)
    #if defined(__AVR_ATmega2560__)
      int size = 8192; // SRAM memory of the arduino mega
      byte *buf;
      while ((buf = (byte *) malloc(--size)) == NULL);
      free(buf);
      Serial.print(F("Free memory: ")); Serial.println(size);
    #elif defined(__XTENSA__)
    #endif
  #endif
}

#if defined(WITH_WIFI)
  float HomeControl::getRSSI() {
    float rssi = WiFi.RSSI();
    rssi = isnan(rssi) ? -100.0 : rssi;
    return min(max(2 * (rssi + 100.0), 0.0), 100.0);
  }
#endif

#if defined(WITH_SERIAL_CONFIG)
  void HomeControl::readSerialInput() {
    char inChar=-1;
    while(Serial.available() > 0) {
      if(serialInIndex < SERIAL_INPUT_BUFFER_SIZE) {
        inChar = Serial.read(); // Read a character
        if (inChar == '\n') {
          serialInData[serialInIndex] = '\0';
          Serial.print(F("Serial command:"));Serial.println(serialInData);
          parseSerialCommand();
          resetSerialInputData();
          return;
        } else {
          serialInData[serialInIndex] = inChar; // Store it
          serialInIndex++; // Increment where to write next
        }
      } else {
        Serial.println(F("Input data too large!"));
        resetSerialInputData();
      }
    }
  }

  void HomeControl::resetSerialInputData() {
    serialInIndex = 0;
    return;
  }

  void HomeControl::parseSerialCommand() {
    // *************** SET CLIENT IP **************//
    if (strstr(serialInData, "client_ip=")) {
      Serial.println(F("Setting Client IP\n"));
      char *token;
      token = strtok(&serialInData[10], ".");
      client_ip[0] = atoi(token);
      token = strtok(NULL, ".");
      client_ip[1] = atoi(token);
      token = strtok(NULL, ".");
      client_ip[2] = atoi(token);    
      token = strtok(NULL, ".");
      client_ip[3] = atoi(token);
      printConfiguration();
    // *************** SET SERVER IP **************//
    } else if (strstr(serialInData, "server_ip=")) {
      Serial.println(F("Setting Server IP\n"));
      char *token;
      token = strtok(&serialInData[10], ".");
      server_ip[0] = atoi(token);
      token = strtok(NULL, ".");
      server_ip[1] = atoi(token);
      token = strtok(NULL, ".");
      server_ip[2] = atoi(token);    
      token = strtok(NULL, ".");
      server_ip[3] = atoi(token);
      printConfiguration();
    // *************** SET MAC ADDRESS **************//
    } else if (strstr(serialInData, "mac=")) {
      Serial.println(F("Setting Server IP\n"));
      char *token;
      token = strtok(&serialInData[4], ":");
      mac[0] = strtoul(token, NULL, 16);
      token = strtok(NULL, ":");
      mac[1] = strtoul(token, NULL, 16);
      token = strtok(NULL, ":");
      mac[2] = strtoul(token, NULL, 16);
      token = strtok(NULL, ":");
      mac[3] = strtoul(token, NULL, 16);
      token = strtok(NULL, ":");
      mac[4] = strtoul(token, NULL, 16);
      token = strtok(NULL, ":");
      mac[5] = strtoul(token, NULL, 16);
      printConfiguration();
    #if defined(ESP8266) || defined(ESP32)
      // *************** SET WIFI SSID 1 ADDRESS **************//
      } else if (strstr(serialInData, "ssid=")) {
        Serial.println(F("Setting Wifi SSID \n"));
        strcpy(wifi_ssid, &serialInData[5]);
        printConfiguration();
      // *************** SET PASS 1 ADDRESS **************//
      } else if (strstr(serialInData, "pass=")) {
        Serial.println(F("Setting Wifi PASS \n"));
        strcpy(wifi_pass, &serialInData[5]);
        printConfiguration();
      // *************** SET SERVER IP **************//
      } else if (strstr(serialInData, "gateway_ip=")) {
        Serial.println(F("Setting Gateway IP\n"));
        char *token;
        token = strtok(&serialInData[11], ".");
        gateway_ip[0] = atoi(token);
        token = strtok(NULL, ".");
        gateway_ip[1] = atoi(token);
        token = strtok(NULL, ".");
        gateway_ip[2] = atoi(token);    
        token = strtok(NULL, ".");
        gateway_ip[3] = atoi(token);
        printConfiguration();
    #endif
    // *************** STORE CONFIGURATION **************//
    } else if (strstr(serialInData, "save")) {
      Serial.println(F("Saving configuration to EEPROM\n"));
      saveConfiguration();
      printConfiguration();
    } else {
      Serial.println(F("Unknown command"));
    }
  }
#endif