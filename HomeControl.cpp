#include "HomeControl.h"

HomeControl::HomeControl() {
  this->device_count = 0;
  this->last_request = millis();
  this->autoreset = 0;
  this->production = 0;
  this->mac[0] = 0x00;
  this->mac[1] = 0xAA;
  this->mac[2] = 0xBB;
  this->mac[3] = 0xCC;
  this->mac[4] = 0xDE;
  this->mac[5] = 0x00;
  this->ip = IPAddress(192, 168, 0, 0);
  this->server_production[0] = 192;
  this->server_production[1] = 168;
  this->server_production[2] = 0;
  this->server_production[3] = 2;

  this->server_development[0] = 192;
  this->server_development[1] = 168;
  this->server_development[2] = 0;
  this->server_development[3] = 100;
  this->ip_offset = 0;
  this->port = 7777;
  
  this->inIndex = 0;
  this->inStatus = 0; // 0 - wait, 1 - command
}

bool HomeControl::setup() {
  pinMode(LED_BUILTIN, OUTPUT); //TEST LED
  pinMode(10, OUTPUT);   // set the Ethernet SS pin as an output (necessary!)
  digitalWrite(10, HIGH);
  
  if (loadConfiguration()) {
    Serial.println(F("Configuration successfull!"));
  } else {
    return false; 
  }
  if (setupEthernet()) {
    Serial.println(F("Ethernet setup successfull!"));
  } else {
    //return false; 
  }
  availableMemory();
  return true;
}

bool HomeControl::loadConfiguration() {
  // IP configuration
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  pinMode(A4, INPUT_PULLUP); // 1 dev mode (no cable) | 0 production mode (cable ground)

  ip_offset = ip_offset + (1 * digitalRead(A0));
  ip_offset = ip_offset + (2 * digitalRead(A1));
  ip_offset = ip_offset + (4 * digitalRead(A2));
  ip_offset = ip_offset + (8 * digitalRead(A3));
  production = !digitalRead(A4);

  // MAC address
  mac[4] = mac[4] + production;
  mac[5] = mac[5] + ip_offset;
  
  // IP address
  if (production) {
    ip[3] = 60 + ip_offset;
  } else {
    ip[3] = 200 + ip_offset;
  }

  // Server
  if (production) {
    for(int i = 0; i < 4; i++) { server[i] = server_production[i]; }
  } else {
    for(int i = 0; i < 4; i++) { server[i] = server_development[i]; }
  }
  printConfiguration();
  return true;
}

bool HomeControl::setupEthernet() {
  Ethernet.begin(mac, ip);
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
  return true;
}

void HomeControl::connect() {
  client.stop();
  Serial.print(F("Connecting to server: "));
  if (client.connect(server, port)) {
    Serial.println(F("OK"));
  } else {
    Serial.println(F("failed"));
  }
}

void HomeControl::addDevice(Device &device) {
  if (device_count >= MAX_DEVICES) {
    Serial.print(F("Maximum devices reached "));Serial.print(MAX_DEVICES);
  } else {
    devices[device_count] = &device;
    device_count = device_count + 1;
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
  while(client.available() > 0) {
    if(inIndex < INPUT_BUFFER_SIZE) {
      inChar = client.read(); // Read a charact
      if (inChar == '\n') {
        inData[inIndex] = '\0';
        Serial.print(F("Received:"));Serial.println(inData);
        parseCommand();
        resetInputData();
        availableMemory();
        return true;
      } else {
        inData[inIndex] = inChar; // Store it
        inIndex++; // Increment where to write next
      }
    } else {
      Serial.println(F("Input data too large!"));
      resetInputData();
    }
  }
}

void HomeControl::resetInputData() {
  //  for(int i=0;i<INPUT_BUFFER_SIZE;i++){
  //    inData[i]=0;
  //  }
  inIndex = 0;
  inStatus = 0;
}

bool HomeControl::parseCommand() {
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
    }
  } else if (doc["reset_devices"]) {
    deleteAllDevices();
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
    Serial.println(F("Uknown command"));
  }
}

void HomeControl::pong() {
  client.write("{\"pong\": true}\n");
  turnOnTestModeLED(500);
}

//void HomeControl::sendData() {
//  DynamicJsonDocument doc(1000);
//  JsonObject device_json_object = doc.createNestedObject("device");
//  for(int i = 0; i < deviceCount; i++) {
//    device.
//    JsonObject channel_data = channel.createNestedObject( String(devices[i]->channel, DEC));
//    if (devices[i]->device_mode == DEVICE_DIGITAL_OUTPUT || devices[i]->device_mode == DEVICE_DIGITAL_INPUT) {
//      JsonObject channel_data = channel.createNestedObject( String(devices[i]->channel, DEC));
//      channel_data["t"] = "b"; /* boolean */
//      channel_data["v"] = devices[i]->bool_value;
//      channel_data["o"] = devices[i]->output();
//      channel_data["p"] = devices[i]->pin;
//    } else if (devices[i]->device_mode == DEVICE_ANALOG_INPUT) {
//      JsonObject channel_data = channel.createNestedObject( String(devices[i]->channel, DEC));
//      channel_data["t"] = "d2"; /*decimal with 2 places */
//      channel_data["v"] = round(devices[i]->float_value * 100L);
//      channel_data["o"] = devices[i]->output();
//      channel_data["p"] = devices[i]->pin;
//    } else if (devices[i]->device_mode == DEVICE_ANALOG_OUTPUT || devices[i]->device_mode == DEVICE_SERVO) {
//      JsonObject channel_data = channel.createNestedObject( String(devices[i]->channel, DEC));
//      channel_data["t"] = "p"; /* percentual integer */
//      channel_data["v"] = devices[i]->int_value;
//      channel_data["o"] = devices[i]->output();
//      channel_data["p"] = devices[i]->pin;
//    } else if (devices[i]->device_mode == DEVICE_DHT22 || devices[i]->device_mode == DEVICE_HDC1080_I2C) {
//      JsonObject channel_data = channel.createNestedObject( String(devices[i]->channel, DEC));
//      channel_data["t"] = "d2"; /*decimal with 2 places */
//      channel_data["v"] = round(devices[i]->temperature * 100L);
//      channel_data["o"] = devices[i]->output();
//      channel_data["p"] = devices[i]->pin;
//      JsonObject channel_data2 = channel.createNestedObject( String(devices[i]->secondary_channel, DEC));
//      channel_data2["t"] = "d2"; /*decimal with 2 places */
//      channel_data2["v"] = round(devices[i]->humidity * 100L);
//      channel_data2["o"] = devices[i]->output();
//      channel_data2["p"] = devices[i]->pin;
//    } else if (devices[i]->device_mode == DEVICE_CCS811_I2C) {
//      JsonObject channel_data = channel.createNestedObject( String(devices[i]->channel, DEC));
//      channel_data["t"] = "d2"; /*decimal with 2 places */
//      channel_data["v"] = round(devices[i]->co2 * 100L);
//      channel_data["o"] = devices[i]->output();
//      channel_data["p"] = devices[i]->pin;
//      JsonObject channel_data2 = channel.createNestedObject( String(devices[i]->secondary_channel, DEC));
//      channel_data2["t"] = "d2"; /*decimal with 2 places */
//      channel_data2["v"] = round(devices[i]->tvoc * 100L);
//      channel_data2["o"] = devices[i]->output();
//      channel_data2["p"] = devices[i]->pin;
//    } else if (devices[i]->device_mode == DEVICE_MG811) {
//      JsonObject channel_data = channel.createNestedObject( String(devices[i]->channel, DEC));
//      channel_data["t"] = "d2"; /*decimal with 2 places */
//      channel_data["v"] = devices[i]->int_value * 100L;
//      channel_data["o"] = devices[i]->output();
//      channel_data["p"] = devices[i]->pin;
//    }
//  }
//  Serial.println(F("Sending data to Automator"));
//  udp.beginPacket(automatorIP, AUTOMATOR_UDP_PORT);
//  //serializeJson(doc, udp);
//  serializeMsgPack(doc, udp);
//  availableMemory();
//  udp.endPacket();  
//}

void HomeControl::loop() {
  if (!client.connected()) {
    Serial.println(F("no connection"));
    delay(5000);
    //resetRelays();
    connect();
  } else {
    readInput();
    
    for(int i = 0; i < device_count; i++) {
      if (!(devices[i]->is_output())) {
        devices[i]->loop();
      }
    }
    
    for(int i = 0; i < device_count; i++) {
      if (devices[i]->report) {
//        serializeJson(devices[i]->sendData(), Serial);
//        Serial.println();
        serializeJson(devices[i]->sendData(), client);
        client.write("\n");
      }
    }

    for(int i = 0; i < device_count; i++) {
      if (!(devices[i]->is_output())) {
        devices[i]->report = false;
      }
    }
    client.flush();
  }
  timer.run();
}

/***** HELP FUNCTIONS ******/

void HomeControl::turnOnTestModeLED(int timeout) {
  digitalWrite(LED_BUILTIN, HIGH);
  timer.setTimeout(timeout, turnOffTestModeLED);
}

static void HomeControl::turnOffTestModeLED() {
  digitalWrite(LED_BUILTIN, LOW);
}

void HomeControl::printConfiguration() {
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
  Serial.print(F("IP:         "));
  Serial.print(ip[0]); Serial.print(F(".")); Serial.print(ip[1]); Serial.print(F(".")); Serial.print(ip[2]); Serial.print(F(".")); Serial.println(ip[3]);
  Serial.print(F("Server IP:  "));
  Serial.print(server[0]); Serial.print("."); Serial.print(server[1]); Serial.print(F(".")); Serial.print(server[2]); Serial.print(F(".")); Serial.println(server[3]);
  Serial.print(F("Port:       ")); Serial.println(port);
}

bool HomeControl::connectionExpired() {
  return (autoreset > 0 && ((last_request + autoreset * 1000L) < millis() || last_request > millis()));
}

static void HomeControl::availableMemory() {
 int size = 8192; // SRAM memory of the arduino
 byte *buf;
 while ((buf = (byte *) malloc(--size)) == NULL);
     free(buf);
 Serial.print(F("Memory: ")); Serial.println(size);
}

//static void HomeControl::parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
//  for (int i = 0; i < maxBytes; i++) {
//    bytes[i] = strtoul(str, NULL, base);  // Convert byte
//    str = strchr(str, sep);               // Find next separator
//    if (str == NULL || *str == '\0') {
//      break;                            // No more separators, exit
//    }
//    str++;                                // Point to next character after separator
//  }
//}
