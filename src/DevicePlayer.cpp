#include "DevicePlayer.h"

DevicePlayer::DevicePlayer(uint32_t device_id) {
  setup();
  this->device_id = device_id;
  // initialize output
  this->player = new MP3();
  delay(500);//Requires 500ms to wait for the MP3 module to initialize  
  this->player->setVolume(5);
  delay(50);
  print();
}

void DevicePlayer::loop() {
}

void DevicePlayer::play(uint8_t directory, uint8_t file) {
  player->playWithFileName(directory, file);
  last_value_change = millis();
  Serial.print(F("Player: ")); Serial.print(F(" play ")); Serial.print(directory); Serial.print("/"); Serial.println(file);
}

void DevicePlayer::volume(uint8_t vol) {
  Serial.print(F("Player: ")); Serial.print(F(" volume ")); Serial.print(vol); Serial.print("/"); Serial.println(23);
  player->setVolume(vol);
}
  
void DevicePlayer::uninitialize() { 
  delete player;
  player = NULL;
}

void DevicePlayer::action(JsonObject doc) {
  if (doc["volume"]) { 
    volume(doc["volume"]);
  }
  if (doc["file"] && doc["directory"]) { 
    play(doc["directory"], doc["file"]);
  }
}

DynamicJsonDocument DevicePlayer::sendData() {
  DynamicJsonDocument doc(500);
  return doc;
}

bool DevicePlayer::is_output() {
  return true;
}

void DevicePlayer::print() {
  Serial.println(F("Player: ")); Serial.print(F(" id:")); Serial.println(device_id);
}