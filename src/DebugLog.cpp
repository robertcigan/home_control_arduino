#include "HomeControl.h"  // For VERSION and WITH_DEBUG_LOG

#include "DebugLog.h"

#if defined(WITH_DEBUG_LOG)
DebugLog::DebugLog() {
  _serverConnected = false;
  _initialized = false;
  _ntpSynced = false;
  _lastStatusLog = 0;
  _server = nullptr;
  _getConfig = nullptr;
  _saveConfig = nullptr;
}

void DebugLog::setConfigCallbacks(GetConfigCallback getConfig, SaveConfigCallback saveConfig) {
  _getConfig = getConfig;
  _saveConfig = saveConfig;
}

bool DebugLog::begin(IPAddress ntpServer) {
  // Initialize LittleFS
  if (!LittleFS.begin()) {
    #if defined(WITH_SERIAL)
      Serial.println(F("[DebugLog] LittleFS mount failed, formatting..."));
    #endif

    // Try to format
    if (LittleFS.format()) {
      if (!LittleFS.begin()) {
        #if defined(WITH_SERIAL)
          Serial.println(F("[DebugLog] LittleFS mount failed after format"));
        #endif
        return false;
      }
    } else {
      #if defined(WITH_SERIAL)
        Serial.println(F("[DebugLog] LittleFS format failed"));
      #endif
      return false;
    }
  }

  #if defined(WITH_SERIAL)
    Serial.println(F("[DebugLog] LittleFS mounted"));
  #endif

  // Setup NTP using built-in library
  setupNTP(ntpServer);

  // Setup web server
  setupWebServer();

  _initialized = true;

  // Log boot event
  logBoot();

  return true;
}

void DebugLog::loop() {
  if (!_initialized) return;

  // Handle web server requests
  if (_server) {
    _server->handleClient();
  }

  // Check if NTP synced (for logging purposes)
  if (!_ntpSynced && isTimeSynced()) {
    _ntpSynced = true;
    #if defined(WITH_SERIAL)
      char timeStr[24];
      formatTime(timeStr, sizeof(timeStr));
      Serial.print(F("[DebugLog] NTP synced: "));
      Serial.println(timeStr);
    #endif
  }

  // Periodic status log (heartbeat)
  if (millis() - _lastStatusLog > STATUS_LOG_INTERVAL) {
    logPeriodicStatus();
    _lastStatusLog = millis();
  }
}

// ==================== NTP Functions (using built-in library) ====================

void DebugLog::setupNTP(IPAddress ntpServer) {
  // Convert IP to string for configTime
  char ntpServerStr[16];
  snprintf(ntpServerStr, sizeof(ntpServerStr), "%d.%d.%d.%d",
           ntpServer[0], ntpServer[1], ntpServer[2], ntpServer[3]);

  #if defined(WITH_SERIAL)
    Serial.print(F("[DebugLog] NTP server: "));
    Serial.println(ntpServerStr);
  #endif

  // Configure NTP - UTC time (offset 0)
  // Using the built-in configTime function
  configTime(0, 0, ntpServerStr, "pool.ntp.org");

  #if defined(WITH_SERIAL)
    Serial.println(F("[DebugLog] NTP configured (UTC)"));
  #endif
}

bool DebugLog::isTimeSynced() {
  time_t now = time(nullptr);
  // Time is synced if it's after year 2020 (timestamp > 1577836800)
  return now > 1577836800;
}

void DebugLog::formatTime(char* buffer, size_t size) {
  time_t now = time(nullptr);

  if (now < 1577836800) {
    // No NTP sync yet, use uptime
    unsigned long uptime = millis() / 1000;
    unsigned long hours = uptime / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;
    snprintf(buffer, size, "UP %02lu:%02lu:%02lu", hours, minutes, seconds);
    return;
  }

  // Use standard library to format time
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
           timeinfo.tm_year + 1900,
           timeinfo.tm_mon + 1,
           timeinfo.tm_mday,
           timeinfo.tm_hour,
           timeinfo.tm_min,
           timeinfo.tm_sec);
}

// ==================== Log Functions ====================

void DebugLog::writeLog(const char* message) {
  if (!_initialized) return;

  // Check if rotation needed
  if (getLogSize() > DEBUG_LOG_MAX_SIZE) {
    rotateLog();
  }

  // Format timestamp
  char timeStr[24];
  formatTime(timeStr, sizeof(timeStr));

  // Get WiFi info
  float rssi = calculateRSSI();
  bool wifiOk = WiFi.status() == WL_CONNECTED;

  // Build log line
  char logLine[200];
  snprintf(logLine, sizeof(logLine), "%s | %3.0f%% | WiFi:%s | TCP:%s | %s\n",
           timeStr,
           rssi,
           wifiOk ? "OK" : "ERR",
           _serverConnected ? "OK" : "ERR",
           message);

  // Append to log file
  File file = LittleFS.open(DEBUG_LOG_FILE, "a");
  if (file) {
    file.print(logLine);
    file.close();

    #if defined(WITH_SERIAL)
      Serial.print(F("[DebugLog] "));
      Serial.print(logLine);
    #endif
  } else {
    #if defined(WITH_SERIAL)
      Serial.println(F("[DebugLog] Failed to open log file"));
    #endif
  }
}

void DebugLog::rotateLog() {
  #if defined(WITH_SERIAL)
    Serial.println(F("[DebugLog] Rotating log file..."));
  #endif

  File srcFile = LittleFS.open(DEBUG_LOG_FILE, "r");
  if (!srcFile) return;

  size_t fileSize = srcFile.size();
  size_t skipBytes = fileSize - DEBUG_LOG_ROTATE_SIZE;

  // Skip to position and find next newline
  srcFile.seek(skipBytes);
  while (srcFile.available()) {
    if (srcFile.read() == '\n') break;
  }

  // Write remaining content to temp file
  File dstFile = LittleFS.open(DEBUG_LOG_TEMP_FILE, "w");
  if (dstFile) {
    while (srcFile.available()) {
      dstFile.write(srcFile.read());
    }
    dstFile.close();
  }
  srcFile.close();

  // Replace original with temp
  LittleFS.remove(DEBUG_LOG_FILE);
  LittleFS.rename(DEBUG_LOG_TEMP_FILE, DEBUG_LOG_FILE);

  #if defined(WITH_SERIAL)
    Serial.print(F("[DebugLog] Log rotated, new size: "));
    Serial.println(getLogSize());
  #endif
}

size_t DebugLog::getLogSize() {
  File file = LittleFS.open(DEBUG_LOG_FILE, "r");
  if (!file) return 0;
  size_t size = file.size();
  file.close();
  return size;
}

// ==================== Log Event Functions ====================

void DebugLog::logEvent(const char* event) {
  writeLog(event);
}

void DebugLog::logBoot() {
  char msg[80];
  #if defined(ESP8266)
    snprintf(msg, sizeof(msg), "BOOT - ESP8266 reset reason: %s", ESP.getResetReason().c_str());
  #elif defined(ESP32)
    snprintf(msg, sizeof(msg), "BOOT - ESP32 reset reason: %d", (int)esp_reset_reason());
  #endif
  writeLog(msg);
}

void DebugLog::logWiFiConnected() {
  char msg[80];
  snprintf(msg, sizeof(msg), "WiFi connected to %s, IP: %s",
           WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  writeLog(msg);
}

void DebugLog::logWiFiDisconnected(int status) {
  char msg[80];
  snprintf(msg, sizeof(msg), "WiFi disconnected, status: %s (%d)",
           getWiFiStatusString(status), status);
  writeLog(msg);
}

void DebugLog::logServerConnected() {
  _serverConnected = true;
  writeLog("Server TCP connected");
}

void DebugLog::logServerDisconnected() {
  _serverConnected = false;
  writeLog("Server TCP disconnected");
}

void DebugLog::logServerTimeout() {
  _serverConnected = false;
  writeLog("Server connection timeout");
}

void DebugLog::logReconnectAttempt(uint8_t attempt, uint32_t delay_ms) {
  char msg[60];
  snprintf(msg, sizeof(msg), "Reconnect attempt #%d, delay: %lums", attempt, (unsigned long)delay_ms);
  writeLog(msg);
}

void DebugLog::logOTAStart(const char* type) {
  char msg[50];
  snprintf(msg, sizeof(msg), "OTA update started: %s", type);
  writeLog(msg);
}

void DebugLog::logOTAEnd(bool success) {
  writeLog(success ? "OTA update successful" : "OTA update failed");
}

void DebugLog::logOTAError(const char* error) {
  char msg[80];
  snprintf(msg, sizeof(msg), "OTA error: %s", error);
  writeLog(msg);
}

void DebugLog::logPeriodicStatus() {
  char msg[120];
  unsigned long uptime_sec = millis() / 1000;
  unsigned long uptime_min = uptime_sec / 60;
  unsigned long uptime_hrs = uptime_min / 60;
  unsigned long uptime_days = uptime_hrs / 24;

  #if defined(ESP32) || defined(ESP8266)
    uint32_t freeHeap = ESP.getFreeHeap();
  #else
    uint32_t freeHeap = 0;
  #endif

  snprintf(msg, sizeof(msg),
           "STATUS: uptime=%lud%luh%lum, heap=%lu, loopOK",
           uptime_days,
           uptime_hrs % 24,
           uptime_min % 60,
           (unsigned long)freeHeap);
  writeLog(msg);
}

void DebugLog::setServerConnected(bool connected) {
  _serverConnected = connected;
}

// ==================== Web Server Functions ====================

bool DebugLog::checkUpdateAuth() {
  #if defined(OTA_UPDATE_PASSWORD) && defined(OTA_UPDATE_USERNAME)
    const char* usr = OTA_UPDATE_USERNAME;
    const char* pwd = OTA_UPDATE_PASSWORD;
    // Require auth only if both username and password are not empty
    if (usr[0] != '\0' && pwd[0] != '\0') {
      if (!_server->authenticate(usr, pwd)) {
        _server->requestAuthentication();
        return false;
      }
    }
  #endif
  return true;
}

void DebugLog::setupWebServer() {
  #if defined(ESP8266)
    _server = new ESP8266WebServer(DEBUG_WEB_PORT);
  #elif defined(ESP32)
    _server = new WebServer(DEBUG_WEB_PORT);
  #endif

  _server->on("/debug", [this]() { handleDebugPage(); });
  _server->on("/debug/clear", [this]() { handleDebugClear(); });
  _server->on("/debug/status", [this]() { handleDebugStatus(); });
  _server->on("/update", HTTP_GET, [this]() {
    if (checkUpdateAuth()) handleUpdatePage();
  });
  _server->on("/update", HTTP_POST,
    [this]() {
      // After upload complete
      _server->sendHeader("Connection", "close");
      if (Update.hasError()) {
        _server->send(500, "text/plain", "Update FAILED! Rebooting...");
      } else {
        _server->send(200, "text/plain", "Update OK! Rebooting...");
      }
      delay(1000);
      ESP.restart();
    },
    [this]() {
      if (checkUpdateAuth()) handleUpdateUpload();
    }
  );
  _server->on("/config", HTTP_GET, [this]() {
    if (checkUpdateAuth()) handleConfigPage();
  });
  _server->on("/config", HTTP_POST, [this]() {
    if (checkUpdateAuth()) handleConfigSave();
  });
  _server->onNotFound([this]() { handleNotFound(); });

  _server->begin();

  #if defined(WITH_SERIAL)
    Serial.print(F("[DebugLog] Web server started on port "));
    Serial.println(DEBUG_WEB_PORT);
  #endif
}

void DebugLog::handleDebugPage() {
  // Use chunked transfer to avoid memory fragmentation
  _server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  _server->send(200, "text/html", "");

  // Send header
  _server->sendContent(F("<!DOCTYPE html><html><head>"));
  _server->sendContent(F("<meta charset='UTF-8'>"));
  _server->sendContent(F("<meta name='viewport' content='width=device-width, initial-scale=1'>"));
  _server->sendContent(F("<title>Debug Log</title>"));
  _server->sendContent(F("<style>"));
  _server->sendContent(F("body{font-family:monospace;background:#1a1a2e;color:#eee;margin:20px;font-size:13px;}"));
  _server->sendContent(F("h1{color:#0f4c75;}"));
  _server->sendContent(F(".btn{background:#0f4c75;color:#fff;padding:10px 20px;text-decoration:none;margin-right:10px;border-radius:3px;display:inline-block;margin-bottom:5px;}"));
  _server->sendContent(F(".btn:hover{background:#1b6ca8;}"));
  _server->sendContent(F(".info{background:#16213e;padding:10px;margin-bottom:15px;border-radius:5px;}"));
  _server->sendContent(F("table{width:100%;border-collapse:collapse;background:#16213e;border-radius:5px;overflow:hidden;}"));
  _server->sendContent(F("th{background:#0f4c75;padding:8px 10px;text-align:left;position:sticky;top:0;}"));
  _server->sendContent(F("td{padding:4px 10px;border-bottom:1px solid #1a1a2e;white-space:nowrap;}"));
  _server->sendContent(F("tr:hover{background:#1f2b4d;}"));
  _server->sendContent(F(".ok{color:#2ecc71;}.err{color:#e74c3c;}"));
  _server->sendContent(F(".log-wrap{max-height:70vh;overflow:auto;border-radius:5px;}"));
  _server->sendContent(F("td:last-child{white-space:normal;word-break:break-word;}"));
  _server->sendContent(F("</style></head><body>"));

  _server->sendContent(F("<h1>ESP Debug Log</h1>"));

  // Current status - build small chunk
  String status = F("<div class='info'><strong>Status:</strong> WiFi: ");
  status += WiFi.status() == WL_CONNECTED ? F("<span class='ok'>Connected</span>") : F("<span class='err'>Disconnected</span>");
  status += F(" | Server: ");
  status += _serverConnected ? F("<span class='ok'>Connected</span>") : F("<span class='err'>Disconnected</span>");
  status += F(" | Signal: ");
  status += String(calculateRSSI(), 0);
  status += F("% | Uptime: ");
  status += String(millis() / 1000);
  status += F("s | Log: ");
  status += String(getLogSize());
  status += F("B</div>");
  _server->sendContent(status);

  // Buttons
  _server->sendContent(F("<p><a class='btn' href='/debug'>&#x21bb; Refresh</a>"));
  _server->sendContent(F("<a class='btn' href='/debug/clear'>&#x1F5D1; Clear</a>"));
  _server->sendContent(F("<a class='btn' href='/debug/status'>{ } JSON</a>"));
  _server->sendContent(F("<a class='btn' href='/update'>&#x1F4E6; Update</a>"));
  _server->sendContent(F("<a class='btn' href='/config'>&#x2699; Config</a></p>"));

  // Log table header
  _server->sendContent(F("<div class='log-wrap'><table><thead><tr>"));
  _server->sendContent(F("<th>Time</th><th>Signal</th><th>WiFi</th><th>TCP</th><th>Event</th>"));
  _server->sendContent(F("</tr></thead><tbody>"));

  // Read and send log file line by line
  File file = LittleFS.open(DEBUG_LOG_FILE, "r");
  if (file) {
    String line = "";
    while (file.available()) {
      char c = file.read();
      if (c == '\n' || !file.available()) {
        if (!file.available() && c != '\n') line += c;

        // Parse line: "2024-12-29 10:30:00 | 75% | WiFi:OK | TCP:OK | Event text"
        if (line.length() > 20) {
          String row = F("<tr>");

          int col = 0;
          int lastPipe = -1;

          for (int i = 0; i <= (int)line.length(); i++) {
            if (i == (int)line.length() || line[i] == '|') {
              String part = line.substring(lastPipe + 1, i);
              part.trim();

              row += F("<td>");
              if (col == 2 || col == 3) {
                // WiFi/TCP status - color code
                if (part.indexOf("OK") >= 0) {
                  row += F("<span class='ok'>");
                  row += part;
                  row += F("</span>");
                } else {
                  row += F("<span class='err'>");
                  row += part;
                  row += F("</span>");
                }
              } else {
                row += part;
              }
              row += F("</td>");

              lastPipe = i;
              col++;
              if (col >= 5) break;
            }
          }

          row += F("</tr>");
          _server->sendContent(row);  // Send each row immediately
        }
        line = "";
      } else {
        line += c;
      }
    }
    file.close();
  } else {
    _server->sendContent(F("<tr><td colspan='5'>(No log file)</td></tr>"));
  }

  _server->sendContent(F("</tbody></table></div></body></html>"));
  _server->sendContent("");  // End chunked transfer
}

void DebugLog::handleDebugClear() {
  LittleFS.remove(DEBUG_LOG_FILE);
  writeLog("Log cleared via web interface");
  _server->sendHeader("Location", "/debug");
  _server->send(302, "text/plain", "Redirecting...");
}

void DebugLog::handleDebugStatus() {
  char timeStr[24];
  formatTime(timeStr, sizeof(timeStr));

  String json = F("{");
  json += F("\"time\":\""); json += timeStr; json += F("\",");
  json += F("\"uptime\":"); json += String(millis() / 1000); json += F(",");
  json += F("\"wifi_connected\":"); json += WiFi.status() == WL_CONNECTED ? F("true") : F("false"); json += F(",");
  json += F("\"wifi_ssid\":\""); json += WiFi.SSID(); json += F("\",");
  json += F("\"wifi_rssi\":"); json += String(calculateRSSI(), 1); json += F(",");
  json += F("\"wifi_status\":"); json += String(WiFi.status()); json += F(",");
  json += F("\"server_connected\":"); json += _serverConnected ? F("true") : F("false"); json += F(",");
  json += F("\"ip\":\""); json += WiFi.localIP().toString(); json += F("\",");
  json += F("\"log_size\":"); json += String(getLogSize()); json += F(",");
  json += F("\"log_max_size\":"); json += String(DEBUG_LOG_MAX_SIZE); json += F(",");
  #if defined(ESP32) || defined(ESP8266)
    json += F("\"free_heap\":"); json += String(ESP.getFreeHeap()); json += F(",");
  #endif
  json += F("\"ntp_synced\":"); json += isTimeSynced() ? F("true") : F("false");
  json += F("}");

  _server->send(200, "application/json", json);
}

void DebugLog::handleNotFound() {
  _server->send(404, "text/plain", "Not found. Try /debug or /update");
}

void DebugLog::handleUpdatePage() {
  String html = F("<!DOCTYPE html><html><head>");
  html += F("<meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  html += F("<title>Firmware Update</title>");
  html += F("<style>");
  html += F("body{font-family:monospace;background:#1a1a2e;color:#eee;margin:20px;text-align:center;}");
  html += F("h1{color:#0f4c75;}");
  html += F(".box{background:#16213e;padding:30px;border-radius:10px;max-width:500px;margin:20px auto;}");
  html += F(".btn{background:#0f4c75;color:#fff;padding:12px 30px;border:none;border-radius:5px;cursor:pointer;font-size:16px;}");
  html += F(".btn:hover{background:#1b6ca8;}");
  html += F(".btn-back{background:#555;margin-top:20px;}");
  html += F("input[type=file]{margin:20px 0;color:#eee;}");
  html += F(".info{background:#0a0a15;padding:15px;border-radius:5px;margin:15px 0;text-align:left;font-size:12px;}");
  html += F(".warn{color:#f39c12;}");
  html += F("progress{width:100%;height:20px;margin-top:10px;}");
  html += F("</style></head><body>");

  html += F("<div class='box'>");
  html += F("<h1>&#128190; Firmware Update</h1>");

  // Current info
  html += F("<div class='info'>");
  html += F("<strong>Current firmware:</strong> v");
  #if defined(VERSION)
    html += String(VERSION);
  #else
    html += F("?");
  #endif
  html += F("<br><strong>Free space:</strong> ");
  #if defined(ESP8266)
    html += String(ESP.getFreeSketchSpace());
  #elif defined(ESP32)
    html += String(ESP.getFreeSketchSpace());
  #endif
  html += F(" bytes<br>");
  html += F("<strong>Chip:</strong> ");
  #if defined(ESP8266)
    html += F("ESP8266");
  #elif defined(ESP32)
    html += ESP.getChipModel();
  #endif
  html += F("</div>");

  // Upload form
  html += F("<form method='POST' action='/update' enctype='multipart/form-data' id='uploadForm'>");
  html += F("<input type='file' name='firmware' accept='.bin' required><br>");
  html += F("<button type='submit' class='btn'>&#128640; Upload Firmware</button>");
  html += F("<progress id='prog' value='0' max='100' style='display:none;'></progress>");
  html += F("<div id='status'></div>");
  html += F("</form>");

  html += F("<p class='warn'>&#9888; Device will reboot after update!</p>");
  html += F("<a href='/debug' class='btn btn-back'>&#8592; Back to Debug</a>");
  html += F("</div>");

  // JavaScript for progress
  html += F("<script>");
  html += F("document.getElementById('uploadForm').addEventListener('submit', function(e) {");
  html += F("  e.preventDefault();");
  html += F("  var form = e.target;");
  html += F("  var formData = new FormData(form);");
  html += F("  var xhr = new XMLHttpRequest();");
  html += F("  var prog = document.getElementById('prog');");
  html += F("  var status = document.getElementById('status');");
  html += F("  prog.style.display = 'block';");
  html += F("  xhr.upload.addEventListener('progress', function(e) {");
  html += F("    if (e.lengthComputable) {");
  html += F("      var pct = Math.round((e.loaded / e.total) * 100);");
  html += F("      prog.value = pct;");
  html += F("      status.innerHTML = 'Uploading: ' + pct + '%';");
  html += F("    }");
  html += F("  });");
  html += F("  xhr.onreadystatechange = function() {");
  html += F("    if (xhr.readyState === 4) {");
  html += F("      if (xhr.status === 200) {");
  html += F("        status.innerHTML = '<b style=\"color:#2ecc71\">Update successful! Rebooting...</b>';");
  html += F("      } else {");
  html += F("        status.innerHTML = '<b style=\"color:#e74c3c\">Update failed: ' + xhr.responseText + '</b>';");
  html += F("      }");
  html += F("    }");
  html += F("  };");
  html += F("  xhr.open('POST', '/update', true);");
  html += F("  xhr.send(formData);");
  html += F("});");
  html += F("</script>");

  html += F("</body></html>");

  _server->send(200, "text/html", html);
}

void DebugLog::handleUpdateUpload() {
  HTTPUpload& upload = _server->upload();

  if (upload.status == UPLOAD_FILE_START) {
    #if defined(WITH_SERIAL)
      Serial.print(F("[OTA-Web] Update start: "));
      Serial.println(upload.filename);
      Serial.print(F("[OTA-Web] Free sketch space: "));
      Serial.println(ESP.getFreeSketchSpace());
    #endif

    writeLog("OTA-Web update started");

    // Close filesystem before update to free resources
    LittleFS.end();

    #if defined(ESP8266)
      // Clear any previous update state
      Update.clearError();
      uint32_t maxSketchSpace = ESP.getFreeSketchSpace();
      if (!Update.begin(maxSketchSpace, U_FLASH)) {
        #if defined(WITH_SERIAL)
          Serial.print(F("[OTA-Web] Update.begin failed: "));
          Update.printError(Serial);
        #endif
      }
    #elif defined(ESP32)
      // Abort any previous update and clear state
      Update.abort();
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
        #if defined(WITH_SERIAL)
          Serial.print(F("[OTA-Web] Update.begin failed: "));
          Update.printError(Serial);
        #endif
      }
    #endif
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      #if defined(WITH_SERIAL)
        Update.printError(Serial);
      #endif
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      #if defined(WITH_SERIAL)
        Serial.print(F("[OTA-Web] Update success, size: "));
        Serial.println(upload.totalSize);
      #endif
    } else {
      #if defined(WITH_SERIAL)
        Serial.print(F("[OTA-Web] Update failed: "));
        Update.printError(Serial);
      #endif
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    #if defined(WITH_SERIAL)
      Serial.println(F("[OTA-Web] Update aborted"));
    #endif
  }
}

void DebugLog::handleConfigPage() {
  if (!_getConfig) {
    _server->send(500, "text/plain", "Config not available");
    return;
  }

  NetworkConfig cfg = _getConfig();

  String html = F("<!DOCTYPE html><html><head>");
  html += F("<meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  html += F("<title>Network Config</title>");
  html += F("<style>");
  html += F("body{font-family:monospace;background:#1a1a2e;color:#eee;margin:20px;}");
  html += F("h1{color:#0f4c75;}");
  html += F("input{width:200px;padding:8px;margin:5px 0;background:#16213e;color:#eee;border:1px solid #0f4c75;}");
  html += F("label{display:inline-block;width:120px;}");
  html += F(".btn{background:#0f4c75;color:#fff;padding:10px 20px;border:none;cursor:pointer;margin-top:15px;}");
  html += F(".btn:hover{background:#1b6ca8;}");
  html += F(".warn{color:#f39c12;margin-top:15px;}");
  html += F("a{color:#4af;}");
  html += F("</style></head><body>");

  html += F("<h1>Network Configuration</h1>");
  html += F("<form method='POST' action='/config'>");

  // Client IP
  html += F("<p><label>Client IP:</label>");
  html += F("<input type='text' name='client_ip' value='");
  html += cfg.client_ip.toString();
  html += F("'></p>");

  // Server IP
  html += F("<p><label>Server IP:</label>");
  html += F("<input type='text' name='server_ip' value='");
  html += cfg.server_ip.toString();
  html += F("'></p>");

  // Gateway IP
  html += F("<p><label>Gateway IP:</label>");
  html += F("<input type='text' name='gateway_ip' value='");
  html += cfg.gateway_ip.toString();
  html += F("'></p>");

  // WiFi SSID
  html += F("<p><label>WiFi SSID:</label>");
  html += F("<input type='text' name='ssid' maxlength='19' value='");
  html += cfg.wifi_ssid;
  html += F("'></p>");

  // WiFi Password
  html += F("<p><label>WiFi Password:</label>");
  html += F("<input type='password' name='pass' maxlength='19' value='");
  html += cfg.wifi_pass;
  html += F("'></p>");

  // MAC Address
  html += F("<p><label>MAC Address:</label>");
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
    cfg.mac[0], cfg.mac[1], cfg.mac[2], cfg.mac[3], cfg.mac[4], cfg.mac[5]);
  html += F("<input type='text' name='mac' value='");
  html += macStr;
  html += F("'></p>");

  html += F("<button type='submit' class='btn'>Save &amp; Reboot</button>");
  html += F("</form>");

  html += F("<p class='warn'>Device will reboot after saving!</p>");
  html += F("<p><a href='/debug'>Back to Debug</a></p>");

  html += F("</body></html>");

  _server->send(200, "text/html", html);
}

void DebugLog::handleConfigSave() {
  if (!_saveConfig) {
    _server->send(500, "text/plain", "Config save not available");
    return;
  }

  NetworkConfig cfg;

  // Parse Client IP
  if (_server->hasArg("client_ip")) {
    cfg.client_ip.fromString(_server->arg("client_ip"));
  }

  // Parse Server IP
  if (_server->hasArg("server_ip")) {
    cfg.server_ip.fromString(_server->arg("server_ip"));
  }

  // Parse Gateway IP
  if (_server->hasArg("gateway_ip")) {
    cfg.gateway_ip.fromString(_server->arg("gateway_ip"));
  }

  // Parse WiFi SSID
  if (_server->hasArg("ssid")) {
    strncpy(cfg.wifi_ssid, _server->arg("ssid").c_str(), 19);
    cfg.wifi_ssid[19] = '\0';
  }

  // Parse WiFi Password
  if (_server->hasArg("pass")) {
    strncpy(cfg.wifi_pass, _server->arg("pass").c_str(), 19);
    cfg.wifi_pass[19] = '\0';
  }

  // Parse MAC Address
  if (_server->hasArg("mac")) {
    String macStr = _server->arg("mac");
    unsigned int m[6];
    if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) == 6) {
      for (int i = 0; i < 6; i++) cfg.mac[i] = m[i];
    }
  }

  // Save config via callback
  _saveConfig(cfg);

  writeLog("Config saved via web, rebooting");

  _server->send(200, "text/html",
    F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
      "<meta http-equiv='refresh' content='5;url=/config'>"
      "<style>body{font-family:monospace;background:#1a1a2e;color:#eee;margin:20px;}</style>"
      "</head><body><h2>Configuration Saved!</h2><p>Rebooting in 3 seconds...</p></body></html>"));

  delay(1000);
  ESP.restart();
}

// ==================== Helper Functions ====================

float DebugLog::calculateRSSI() {
  float rssi = WiFi.RSSI();
  rssi = isnan(rssi) ? -100.0 : rssi;
  return min(max(2 * (rssi + 100.0), 0.0), 100.0);
}

bool DebugLog::isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool DebugLog::isServerConnected() {
  return _serverConnected;
}

float DebugLog::getRSSI() {
  return calculateRSSI();
}

const char* DebugLog::getWiFiStatusString(int status) {
  switch (status) {
    case WL_CONNECTED:      return "CONNECTED";
    case WL_DISCONNECTED:   return "DISCONNECTED";
    case WL_IDLE_STATUS:    return "IDLE";
    case WL_NO_SSID_AVAIL:  return "NO_SSID_AVAIL";
    case WL_CONNECT_FAILED: return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:return "CONNECTION_LOST";
    case WL_SCAN_COMPLETED: return "SCAN_COMPLETED";
    default:                return "UNKNOWN";
  }
}
#endif