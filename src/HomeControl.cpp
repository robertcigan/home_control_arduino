#include "HomeControl.h"

#if defined(WITH_DEBUG_LOG)
  HomeControl* HomeControl::_instance = nullptr;
#endif

HomeControl::HomeControl() {
  #if defined(WITH_DEBUG_LOG)
    _instance = this;
  #endif
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
    this->wifi_reconnect_attempt = 0;
    this->last_wifi_check = 0;
    this->wifi_connecting = false;
    this->reconnect_attempts = 0;
    #if defined(WITH_DEBUG_LOG)
      this->lastWiFiConnected = false;
      this->lastServerConnected = false;
    #endif
  #endif

  // Connection timing (used for all platforms)
  this->last_connection_attempt = 0;

  // Debug timing variables
  this->last_debug_message = 0;
  this->last_skip_message = 0;

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

    #if defined(WITH_DEBUG_LOG)
      // Initialize debug log with gateway as NTP server
      if (debugLog.begin(gateway_ip)) {
        #if defined(WITH_SERIAL)
          Serial.println(F("Debug log initialized"));
        #endif
        // Set up config callbacks for web interface
        debugLog.setConfigCallbacks(getNetworkConfig, saveNetworkConfig);
        lastWiFiConnected = (WiFi.status() == WL_CONNECTED);
        debugLog.setServerConnected(false);
      } else {
        #if defined(WITH_SERIAL)
          Serial.println(F("Debug log initialization failed"));
        #endif
      }
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

    // Disable WiFi persistence to prevent flash wear on all WiFi platforms
    WiFi.persistent(false);
    // Disable auto-reconnect - we have our own reconnection logic
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_STA);

    // Configure static IP before connecting
    WiFi.config(client_ip, gateway_ip, gateway_ip);

    // Start WiFi connection
    WiFi.begin(wifi_ssid, wifi_pass);

    // Platform-specific power management configuration
    #if defined(ESP32)
      esp_wifi_set_ps(WIFI_PS_NONE);
      #if defined(WITH_SERIAL)
        Serial.println(F("ESP32: WiFi power saving disabled"));
      #endif
    #elif defined(ESP8266)
      WiFi.setSleepMode(WIFI_NONE_SLEEP);
      #if defined(WITH_SERIAL)
        Serial.println(F("ESP8266: WiFi sleep mode disabled"));
      #endif
    #endif

    #if defined(ARDUINO_LOLIN_C3_MINI)
      WiFi.setTxPower(WIFI_POWER_8_5dBm); //https://forum.arduino.cc/t/no-wifi-connect-with-esp32-c3-super-mini/1324046/12
      #if defined(WITH_SERIAL)
        Serial.println(F("ESP32-C3: TX power set to 8.5dBm"));
      #endif
    #endif

    // Wait for initial connection with timeout
    uint32_t startTime = millis();
    #if defined(WITH_SERIAL)
      Serial.print(F("WiFi connecting"));
    #endif

    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 10000) {
      delay(100);  // Shorter delay than original 500ms
      #if defined(WITH_SERIAL)
        Serial.print(".");
      #endif
    }

    if (WiFi.status() == WL_CONNECTED) {
      #if defined(WITH_SERIAL)
        Serial.println(F("\nWiFi connected successfully"));
        Serial.print(F("IP: ")); Serial.println(WiFi.localIP());
        Serial.print(F("SSID: ")); Serial.println(WiFi.SSID());
        Serial.print(F("Signal: ")); Serial.print(getRSSI()); Serial.println(F("%"));
      #endif

      return true;
    } else {
      #if defined(WITH_SERIAL)
        Serial.println(F("\nWiFi connection failed in setup"));
        Serial.print(F("WiFi status: ")); Serial.println(WiFi.status());
      #endif
      return false;
    }
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
      return;
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
  setNetwork(); // set network again if W5500 loses data due to reset or so

  #if defined(__AVR_ATmega2560__)
    // Only stop if actually connected
    if (client.connected()) {
      client.stop();
      delay(50);  // Short delay to release socket
    }

    #if defined(WITH_SERIAL)
      printTimestamp();
      Serial.print(F("[TCP] Connecting to server: "));
      Serial.print(server_ip);
      Serial.print(F(":"));
      Serial.print(port);
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

  #elif defined(WITH_WIFI)
    // Check WiFi status first
    if (WiFi.status() != WL_CONNECTED) {
      #if defined(WITH_SERIAL)
        Serial.print(F("WiFi disconnected (status: "));
        Serial.print(WiFi.status());
        Serial.print(F("), reconnecting: "));
      #endif

      // Non-blocking WiFi reconnection
      if (!wifi_connecting) {
        WiFi.disconnect();
        delay(10);  // Short delay for stability between disconnect and begin
        WiFi.begin(wifi_ssid, wifi_pass);
        wifi_connecting = true;
        wifi_reconnect_attempt = millis();
        #if defined(WITH_SERIAL)
          Serial.println(F("started"));
        #endif
      }

      // Check if WiFi reconnection succeeded or timed out
      if (millis() - wifi_reconnect_attempt > 10000) {
        // Timeout, reset attempt
        wifi_connecting = false;
        #if defined(WITH_SERIAL)
          Serial.println(F("WiFi reconnection timeout"));
        #endif
        return;
      }

      if (WiFi.status() != WL_CONNECTED) {
        return; // Still connecting, try again next loop
      }

      wifi_connecting = false;
      #if defined(WITH_SERIAL)
        Serial.print(F("WiFi reconnected: "));
        Serial.println(WiFi.SSID());
        Serial.print(F("Signal: "));
        Serial.print(getRSSI());
        Serial.println(F("%"));
      #endif

      #if defined(WITH_DEBUG_LOG)
        debugLog.logWiFiConnected();
        lastWiFiConnected = true;
      #endif
    }

    // Now handle TCP connection - only stop if actually connected
    if (client.connected()) {
      client.stop();
      delay(50);  // Short delay to release socket
    }

    #if defined(WITH_SERIAL)
      Serial.print(F("Connecting to server: "));
      Serial.print(server_ip);
      Serial.print(F(":"));
      Serial.print(port);
      Serial.print(F(" "));
    #endif

    if (client.connect(server_ip, port)) {
      #if defined(WITH_SERIAL)
        Serial.println(F("OK"));
      #endif

      // Configure TCP options for better connection stability
      #if defined(ESP8266)
        client.setNoDelay(true);
        client.keepAlive(10, 5, 3);  // idle=10s, interval=5s, count=3
      #elif defined(ESP32)
        client.setNoDelay(true);
      #endif

      last_request = millis();
      reconnect_attempts = 0;  // Reset backoff counter on success

      #if defined(WITH_DEBUG_LOG)
        debugLog.logServerConnected();
        lastServerConnected = true;
      #endif

      // Wait a bit before sending device registration to let connection stabilize
      if (!devicesSet) {
        delay(100); // Small delay to let connection stabilize
        sendDevices();
      }
    } else {
      reconnect_attempts++;  // Increment for exponential backoff
      #if defined(WITH_SERIAL)
        Serial.println(F("TCP connection failed"));
      #endif

      #if defined(WITH_DEBUG_LOG)
        debugLog.logReconnectAttempt(reconnect_attempts, getReconnectDelay());
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
          printTimestamp();
          Serial.print(F("[TCP] Received: "));Serial.println(inData);
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
        DeviceCurtain *device = new DeviceCurtain(doc["add"]["id"], doc["add"]["open_pin"], doc["add"]["close_pin"], doc["add"]["inverted"]);
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
  DynamicJsonDocument doc(250);
  doc["pong"] = true;
  doc["version"] = VERSION;
  doc["devices"] = device_count;
  doc["uptime"] = millis() / 1000;
  #if defined(ESP32) || defined(ESP8266)
    doc["free_memory"] = ESP.getFreeHeap();
  #endif

  #if defined(WITH_WIFI)
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = getRSSI();
    doc["wifi_status"] = WiFi.status();
    doc["ip"] = WiFi.localIP().toString();
  #endif

  #if defined(WITH_SERIAL)
    Serial.print(F("[PING] Received server ping, responding: "));
    serializeJson(doc, Serial);
    Serial.println();
    #if defined(WITH_WIFI)
      printWiFiStatus();
    #endif
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

  #if defined(WITH_DEBUG_LOG)
    debugLog.loop();
  #endif

  #if defined(WITH_WIFI)
    // Check WiFi status periodically (every 5 seconds, half of server ping interval)
    if (millis() - last_wifi_check > 5000) {
      last_wifi_check = millis();

      #if defined(WITH_DEBUG_LOG)
        // Log WiFi status changes
        bool currentWiFiConnected = (WiFi.status() == WL_CONNECTED);
        if (lastWiFiConnected && !currentWiFiConnected) {
          debugLog.logWiFiDisconnected(WiFi.status());
        } else if (!lastWiFiConnected && currentWiFiConnected) {
          debugLog.logWiFiConnected();
        }
        lastWiFiConnected = currentWiFiConnected;
      #endif

      if (WiFi.status() != WL_CONNECTED) {
        #if defined(WITH_SERIAL)
          printTimestamp();
          Serial.print(F("[WiFi] Connection lost (status: "));
          Serial.print(WiFi.status());
          Serial.println(F(")"));
        #endif
        // Force reconnection
        connect();
        return;
      }
    }
  #endif

  // Check both client connection and timeout
  // Optimization: only check timeout if connected (no point checking if already disconnected)
  bool is_disconnected = !client.connected();
  bool is_expired = is_disconnected ? false : connectionExpired();
  bool need_reconnect = is_disconnected || is_expired;

  #if defined(WITH_DEBUG_LOG)
    // Log server connection status changes
    if (need_reconnect && lastServerConnected) {
      if (is_expired) {
        debugLog.logServerTimeout();
      } else {
        debugLog.logServerDisconnected();
      }
      lastServerConnected = false;
    }
  #endif

  if (need_reconnect) {
    // Only print debug message once per reconnection cycle (using member variable)
    if (millis() - last_debug_message > 1000) { // Limit debug messages to once per second
      last_debug_message = millis();

      #if defined(WITH_SERIAL)
        printTimestamp();
        if (is_disconnected) {
          Serial.print(F("[TCP] Client disconnected (uptime: "));
        } else {
          Serial.print(F("[TIMEOUT] Connection expired (uptime: "));
        }
        Serial.print(millis() / 1000);
        Serial.print(F("s, last_request: "));
        Serial.print((millis() - last_request) / 1000);
        Serial.println(F("s ago)"));
      #endif
    }

    #if defined(WITH_LED)
      turnOnTestModeLED(0);
    #endif

    // Add delay between reconnection attempts using exponential backoff
    #if defined(WITH_WIFI)
      uint32_t reconnect_delay = getReconnectDelay();
    #else
      uint32_t reconnect_delay = 2000;  // Fixed 2s for Ethernet
    #endif

    if (millis() - last_connection_attempt > reconnect_delay) {
      last_connection_attempt = millis();
      connect();
    } else {
      #if defined(WITH_SERIAL)
        // Using member variable instead of static
        if (millis() - last_skip_message > 3000) { // Only print this message every 3 seconds
          printTimestamp();
          Serial.print(F("[TCP] Waiting "));
          Serial.print((reconnect_delay - (millis() - last_connection_attempt)) / 1000);
          Serial.println(F("s before next reconnection attempt"));
          last_skip_message = millis();
        }
      #endif
    }
  } else {
    readInput();
    loopDevices();
    reportDevices();
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
        printTimestamp();
        Serial.print(F("[DEVICE] Sending: "));
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
    #elif defined(ESP32) || defined(ESP8266)
    #endif
  #endif
}

#if defined(WITH_WIFI)
  float HomeControl::getRSSI() {
    float rssi = WiFi.RSSI();
    rssi = isnan(rssi) ? -100.0 : rssi;
    return min(max(2 * (rssi + 100.0), 0.0), 100.0);
  }

  void HomeControl::printWiFiStatus() {
    #if defined(WITH_SERIAL)
      printTimestamp();
      Serial.print(F("[WiFi] Status: "));
      switch (WiFi.status()) {
        case WL_CONNECTED:     Serial.print(F("CONNECTED")); break;
        case WL_DISCONNECTED:  Serial.print(F("DISCONNECTED")); break;
        case WL_IDLE_STATUS:   Serial.print(F("IDLE")); break;
        case WL_NO_SSID_AVAIL: Serial.print(F("NO_SSID_AVAIL")); break;
        case WL_CONNECT_FAILED:Serial.print(F("CONNECT_FAILED")); break;
        case WL_CONNECTION_LOST:Serial.print(F("CONNECTION_LOST")); break;
        case WL_SCAN_COMPLETED:Serial.print(F("SCAN_COMPLETED")); break;
        default:               Serial.print(WiFi.status()); break;
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print(F(", SSID: ")); Serial.print(WiFi.SSID());
        Serial.print(F(", IP: ")); Serial.print(WiFi.localIP());
        Serial.print(F(", Signal: ")); Serial.print(getRSSI()); Serial.print(F("%"));
        Serial.print(F(", Uptime: ")); Serial.print(millis() / 1000); Serial.print(F("s"));
      }
      Serial.println();
    #endif
  }

  uint32_t HomeControl::getReconnectDelay() {
    // Exponential backoff: 2s, 4s, 8s, 16s, max 30s
    // After successful connection, reconnect_attempts is reset to 0
    uint32_t delay_ms = 2000UL << min(reconnect_attempts, (uint8_t)4);
    uint32_t max_delay = 30000UL;

    #if defined(WITH_SERIAL)
      if (reconnect_attempts > 0) {
        printTimestamp();
        Serial.print(F("[TCP] Backoff attempt #"));
        Serial.print(reconnect_attempts);
        Serial.print(F(", delay: "));
        Serial.print(min(delay_ms, max_delay) / 1000);
        Serial.println(F("s"));
      }
    #endif

    return min(delay_ms, max_delay);
  }

#endif

void HomeControl::printTimestamp() {
  #if defined(WITH_SERIAL)
    uint32_t seconds = millis() / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    Serial.print(F("["));
    if (hours < 10) Serial.print(F("0"));
    Serial.print(hours % 24);
    Serial.print(F(":"));
    if ((minutes % 60) < 10) Serial.print(F("0"));
    Serial.print(minutes % 60);
    Serial.print(F(":"));
    if ((seconds % 60) < 10) Serial.print(F("0"));
    Serial.print(seconds % 60);
    Serial.print(F("] "));
  #endif
}

#if defined(WITH_DEBUG_LOG)
  NetworkConfig HomeControl::getNetworkConfig() {
    NetworkConfig cfg;
    if (_instance) {
      cfg.client_ip = _instance->client_ip;
      cfg.server_ip = _instance->server_ip;
      #if defined(WITH_WIFI)
        cfg.gateway_ip = _instance->gateway_ip;
        strncpy(cfg.wifi_ssid, _instance->wifi_ssid, 19);
        cfg.wifi_ssid[19] = '\0';
        strncpy(cfg.wifi_pass, _instance->wifi_pass, 19);
        cfg.wifi_pass[19] = '\0';
      #endif
      memcpy(cfg.mac, _instance->mac, 6);
    }
    return cfg;
  }

  void HomeControl::saveNetworkConfig(NetworkConfig& cfg) {
    if (!_instance) return;

    _instance->client_ip = cfg.client_ip;
    _instance->server_ip = cfg.server_ip;
    #if defined(WITH_WIFI)
      _instance->gateway_ip = cfg.gateway_ip;
      strncpy(_instance->wifi_ssid, cfg.wifi_ssid, 19);
      _instance->wifi_ssid[19] = '\0';
      strncpy(_instance->wifi_pass, cfg.wifi_pass, 19);
      _instance->wifi_pass[19] = '\0';
    #endif
    memcpy(_instance->mac, cfg.mac, 6);

    _instance->saveConfiguration();

    #if defined(WITH_SERIAL)
      Serial.println(F("[Config] Network config saved via web"));
      _instance->printConfiguration();
    #endif
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