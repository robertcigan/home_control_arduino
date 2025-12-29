#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <Arduino.h>
#include <time.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <LittleFS.h>
  #include <Updater.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <LittleFS.h>
  #include <Update.h>
#endif

// Log file settings
#define DEBUG_LOG_FILE "/debug.log"
#define DEBUG_LOG_TEMP_FILE "/debug.tmp"

#if defined(ARDUINO_ESP8266_ESP01)
  #define DEBUG_LOG_MAX_SIZE 32768    // 32KB for ESP-01
#else
  #define DEBUG_LOG_MAX_SIZE 102400   // 100KB for other ESP
#endif

#define DEBUG_LOG_ROTATE_SIZE (DEBUG_LOG_MAX_SIZE / 2)  // Keep half when rotating

// Periodic status log interval
#define STATUS_LOG_INTERVAL 300000  // 5 minutes (ms)

// Web server port
#define DEBUG_WEB_PORT 80

class DebugLog {
  public:
    DebugLog();

    // Initialize the debug log system
    bool begin(IPAddress ntpServer);

    // Main loop - handles web server
    void loop();

    // Log events
    void logEvent(const char* event);
    void logWiFiConnected();
    void logWiFiDisconnected(int status);
    void logServerConnected();
    void logServerDisconnected();
    void logServerTimeout();
    void logReconnectAttempt(uint8_t attempt, uint32_t delay_ms);
    void logOTAStart(const char* type);
    void logOTAEnd(bool success);
    void logOTAError(const char* error);
    void logBoot();
    void logPeriodicStatus();

    // Get current status
    bool isWiFiConnected();
    bool isServerConnected();
    float getRSSI();

    // Set connection status (called from HomeControl)
    void setServerConnected(bool connected);

  private:
    #if defined(ESP8266)
      ESP8266WebServer* _server;
    #elif defined(ESP32)
      WebServer* _server;
    #endif

    bool _serverConnected;
    bool _initialized;
    bool _ntpSynced;
    uint32_t _lastStatusLog;     // Last periodic status log time

    // Time functions
    void setupNTP(IPAddress ntpServer);
    bool isTimeSynced();
    void formatTime(char* buffer, size_t size);

    // Log file functions
    void writeLog(const char* message);
    void rotateLog();
    size_t getLogSize();

    // Web server handlers
    void setupWebServer();
    bool checkUpdateAuth();
    void handleDebugPage();
    void handleDebugClear();
    void handleDebugStatus();
    void handleUpdatePage();
    void handleUpdateUpload();
    void handleNotFound();

    // Helper functions
    float calculateRSSI();
    const char* getWiFiStatusString(int status);
};

#endif
