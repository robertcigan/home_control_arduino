#include "HomeControl.h"

HomeControl::HomeControl() {
  this->devicesSet = false;
  this->device_count = 0;
  this->last_request = millis();
  this->autoreset = 0;
  this->mac[0] = 0xDE;
  this->mac[1] = 0xAA;
  this->mac[2] = 0xBB;
  this->mac[3] = 0xCC;
  this->mac[4] = 0xDE;
  this->mac[5] = 0x00;
  this->client_ip = IPAddress(192, 168, 0, 0);
  this->server_ip = IPAddress(192, 168, 0, 0 );
  this->port = 7777;
  
  this->inIndex = 0;
  this->inStatus = 0; // 0 - wait, 1 - command

  this->serialInIndex = 0; 
}

bool HomeControl::setup() {
  #if defined(__AVR_ATmega2560__)
    pinMode(LED_BUILTIN, OUTPUT); //TEST LED
    pinMode(10, OUTPUT);   // set the Ethernet SS pin as an output (necessary!)
    digitalWrite(10, HIGH);
  #elif defined(__XTENSA__)
    EEPROM.begin(512);
  #endif
  
  if (loadConfiguration()) {
    Serial.println(F("Configuration successfull!"));
  } else {
    return false; 
  }
    if (setupNetwork()) {
      Serial.println(F("Network setup successfull!"));
    } else {
      //return false; 
    }
  availableMemory();
  return true;
}

bool HomeControl::loadConfiguration() {
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
  } else {
    Serial.println("EEPROM not initialized, storing default values now.");
    #if defined(__AVR_ATmega2560__)
      EEPROM.update(EEPROM_CONFIG_SET_OFFSET, EEPROM_INITIALIZED_VALUE);
      EEPROM.update(EEPROM_CONFIG_SET_OFFSET + 1, EEPROM_INITIALIZED_VALUE);
    #elif defined(__XTENSA__)
      EEPROM.write(EEPROM_CONFIG_SET_OFFSET, EEPROM_INITIALIZED_VALUE);
      EEPROM.write(EEPROM_CONFIG_SET_OFFSET + 1, EEPROM_INITIALIZED_VALUE);
    #endif
    saveConfiguration();
  }
  printConfiguration();
  return true;
}

bool HomeControl::saveConfiguration() {
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
  #elif defined(__XTENSA__)
    for(int i = 0; i <= 3; i++) {
      EEPROM.write(i + EEPROM_CLIENT_IP_OFFSET, client_ip[i]);
    }
    for(int i = 0; i <= 3; i++) {
      EEPROM.write(i + EEPROM_SERVER_IP_OFFSET, server_ip[i]);
    }
    for(int i = 0; i <= 5; i++) {
      EEPROM.write(i + EEPROM_MAC_OFFSET, mac[i]);
    }
    if (EEPROM.commit()) {
      Serial.println(F("EEPROM successfully committed"));
    } else {
      Serial.println(F("ERROR! EEPROM commit failed"));
    }
  #endif
  return true;
}

bool HomeControl::setupNetwork() {
  #if defined(__AVR_ATmega2560__)
    Ethernet.begin(mac, client_ip);
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println(F("Ethernet shield was not found."));
      return false;
    } else if (Ethernet.hardwareStatus() == EthernetW5100) {
      Serial.println(F("W5100 Ethernet controller detected."));
    } else if (Ethernet.hardwareStatus() == EthernetW5200) {
      Serial.println(F("W5200 Ethernet controller detected."));
    } else if (Ethernet.hardwareStatus() == EthernetW5500) {
      Serial.println(F("W5500 Ethernet controller detected."));
    }
    if (Ethernet.linkStatus() == Unknown) {
      Serial.println(F("Link status unknown. Link status detection is only available with W5200 and W5500."));
    } else if (Ethernet.linkStatus() == LinkON) {
      Serial.println(F("Link status: On"));
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println(F("Link status: Off"));
    }
  #elif defined(__XTENSA__)
    Serial.setDebugOutput(true);
    wifiMulti.addAP("RC24", "");
    wifiMulti.addAP("CQRC", "");
    WiFi.mode(WIFI_STA);
    IPAddress gateway_ip(192, 168, 0, 1);
    WiFi.config(client_ip, gateway_ip, gateway_ip);
  #endif
  return true;
}

void HomeControl::connect() {
  #if defined(__AVR_ATmega2560__)
    client.stop();
    Serial.print(F("Connecting to server: "));
    if (client.connect(server_ip, port)) {
      Serial.println(F("OK"));
      if (!devicesSet) {
        sendDevices();
      }
    } else {
      Serial.println(F("failed"));
    }
  #elif defined(__XTENSA__)
    client.stop();
    Serial.print(F("Connecting to WiFi: "));
    if (wifiMulti.run(5000) == WL_CONNECTED) {
      Serial.print(F("WiFi connected: "));
      Serial.print(WiFi.SSID());
      Serial.print(F("Connecting to server: "));
      Serial.print(F(" "));
      if (client.connect(server_ip, port)) {
        Serial.println(F("OK"));
      } else {
        Serial.println(F("failed"));
      }
    } else {
      Serial.println(F("WiFi not connected!"));
    }
  #endif
}

void HomeControl::addDevice(Device &device) {
  if (device_count >= MAX_DEVICES) {
    Serial.print(F("Maximum devices reached "));Serial.print(MAX_DEVICES);
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
        Serial.print(F("Received:"));Serial.println(inData);
        parseCommand();
        resetInputData();
        availableMemory();
        return;
      } else {
        inData[inIndex] = inChar; // Store it
        inIndex++; // Increment where to write next
      }
    } else {
      Serial.println(F("Input data overflow!"));
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
    Serial.print(F("deserializeJson() failed with code ")); Serial.println(err.c_str());
  }
  if (doc["add"]) {
    if (doc["add"]["type"] == F("switch")) {
      DeviceSwitch *device = new DeviceSwitch(doc["add"]["id"], doc["add"]["pin"]);
      addDevice(*device);
    } else if (doc["add"]["type"] == F("button")) {
      DeviceButton *device = new DeviceButton(doc["add"]["id"], doc["add"]["pin"]);
      addDevice(*device);
    } else if (doc["add"]["type"] == F("relay")) {
      DeviceRelay *device = new DeviceRelay(doc["add"]["id"], doc["add"]["pin"]);
      addDevice(*device);
    } else if (doc["add"]["type"] == F("player")) {
      DevicePlayer *device = new DevicePlayer(doc["add"]["id"].as<uint32_t>());
      addDevice(*device);
    } else if (doc["add"]["type"] == F("distance")) {
      DeviceDistance *device = new DeviceDistance(doc["add"]["id"], doc["add"]["write_pin"], doc["add"]["read_pin"]);
      addDevice(*device);
    } else if (doc["add"]["type"] == F("analog_input")) {
      DeviceAnalogInput *device = new DeviceAnalogInput(doc["add"]["id"], doc["add"]["apin"]);
      addDevice(*device);
    } else if (doc["add"]["type"] == F("ds18b20")) {
      DeviceDS18B20 *device = new DeviceDS18B20(doc["add"]["id"], doc["add"]["pin"]);
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
    Serial.println(F("Unknown command"));
  }
}

void HomeControl::pong() {
  client.write("{\"pong\": true}\n");
  turnOnTestModeLED(500);
}

void HomeControl::sendDevices() {
  client.write("{\"send_devices\": true}\n");
}

void HomeControl::loop() {
  readSerialInput();
  if (!client.connected()) {
    delay(5000);
    connect();
  } else {
    readInput();
    
    // loop through all input devices to read their data
    for(int i = 0; i < device_count; i++) {
      if (!(devices[i]->is_output())) {
        devices[i]->loop();
      }
    }
    
    // if ther's value to report to server from devices, sent it
    for(int i = 0; i < device_count; i++) {
      if (devices[i]->report && devices[i]->value_initialized) {
        Serial.print(F("Sending: "));
        serializeJson(devices[i]->sendData(), Serial);
        Serial.println();
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
  timer.run();
}

/***** HELP FUNCTIONS ******/

void HomeControl::turnOnTestModeLED(int timeout) {
  digitalWrite(LED_BUILTIN, HIGH);
  timer.setTimeout(timeout, turnOffTestModeLED);
}

void HomeControl::turnOffTestModeLED() {
  digitalWrite(LED_BUILTIN, LOW);
}

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
  Serial.print(F("Client IP:  "));
  Serial.print(client_ip[0]); Serial.print(F(".")); Serial.print(client_ip[1]); Serial.print(F(".")); Serial.print(client_ip[2]); Serial.print(F(".")); Serial.println(client_ip[3]);
  Serial.print(F("Server IP:  "));
  Serial.print(server_ip[0]); Serial.print("."); Serial.print(server_ip[1]); Serial.print(F(".")); Serial.print(server_ip[2]); Serial.print(F(".")); Serial.println(server_ip[3]);
  Serial.print(F("Port:       ")); Serial.println(port);
  Serial.println(F("Available commands:"));
  Serial.println(F("  server_ip=123.123.123.123"));
  Serial.println(F("  client_ip=123.123.123.123"));
  Serial.println(F("  mac=DE:AD:FF:FF:FF:FF"));
  Serial.println(F("  save"));
}

bool HomeControl::connectionExpired() {
  return (autoreset > 0 && ((last_request + autoreset * 1000L) < millis() || last_request > millis()));
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
  // *************** STORE CONFIGURATION **************//
  } else if (strstr(serialInData, "save")) {
    Serial.println(F("Saving configuration to EEPROM\n"));
    saveConfiguration();
    printConfiguration();     
  } else {
    Serial.println(F("Unknown command"));
  }
}