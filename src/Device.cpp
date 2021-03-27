#include "Device.h"

Device::Device() {
  this->setup();
}

Device::~Device() {
}

void Device::setup() {
  value_initialized = false;
  poll = 0;
  last_run = millis();
  last_value_change = millis();
  report = false;
}

bool Device::is_output() {
  return false;
}

void Device::convertToAnalogPin() {
  #if defined(__AVR__)
    switch (pin) {
      case 0: apin = A0; break;
      case 1: apin = A1; break;
      case 2: apin = A2; break;
      case 3: apin = A3; break;
      case 4: apin = A4; break;
      case 5: apin = A5; break;
      case 6: apin = A6; break;
      case 7: apin = A7; break;
      case 8: apin = A8; break;
      case 9: apin = A9; break;
      case 10: apin = A10; break;
      case 11: apin = A11; break;
      case 12: apin = A12; break;
      case 13: apin = A13; break;
      case 14: apin = A14; break;
      case 15: apin = A15; break;
      break;
    }

  #elif defined(ESP8266)
    switch (pin) {
      case 0: apin = A0; break;
      break;
    }
  #endif
}

float Device::ADConversion(int input) {
  return input * 5.0 / 1024;
}
int Device::PercentualPWMConversion(int input) { /* 0-100 to 0-255 */
  return input * 255 / 100;
}
int Device::PercentualServoConversion(int input) { /* 0-100 to 0-180 */
  return input * 180 / 100;
}
