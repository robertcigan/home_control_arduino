#include <Arduino.h>

#define RELAY_1 2
#define RELAY_2 3
#define MANUAL_1 A3
#define MANUAL_2 A4
#define EXTERNAL_1 A1
#define EXTERNAL_2 A2
#define EXTERNAL_INVERT true
#define FULL_ACTION_TIME 70e3
#define FULL_CONTROL_TIME 3e3

uint8_t status = 0; // 0 - wait, 1 - open, 2 - close, 3 - full open, 4 - full close, 11 - external open, 12 - external close
uint32_t actionStarted;

void openCurtain() {
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, HIGH);
}

void closeCurtain() {
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
}

void stopCurtain() {
  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, HIGH);
}

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  stopCurtain();
  pinMode(MANUAL_1, INPUT_PULLUP);
  pinMode(MANUAL_2, INPUT_PULLUP);
  pinMode(EXTERNAL_1, INPUT_PULLUP);
  pinMode(EXTERNAL_2, INPUT_PULLUP);
}

void loop() {
  //Serial.print(status);
  if (status == 0) {
    if (digitalRead(MANUAL_1) == LOW) {
      Serial.println("Manual Open");
      status = 1;
      actionStarted = millis();
      openCurtain();
      delay(200);
    } else if (digitalRead(MANUAL_2) == LOW) {
      Serial.println("Manual Close");
      status = 2;
      actionStarted = millis();
      closeCurtain();
      delay(200);
    } else if (digitalRead(EXTERNAL_1) == (EXTERNAL_INVERT ? HIGH : LOW)) {
      Serial.println("External Open");
      status = 11;
      actionStarted = millis();
      openCurtain();
      delay(200);
    } else if (digitalRead(EXTERNAL_2) == (EXTERNAL_INVERT ? HIGH : LOW)) {
      Serial.println("External Close");
      status = 12;
      actionStarted = millis();
      closeCurtain();
      delay(200);
    }
  } else if (status == 1) {
    // OPENING
    if (digitalRead(MANUAL_1) == HIGH) {
      status = 0;
      stopCurtain();
    } else if ((millis() - actionStarted) > FULL_CONTROL_TIME) {
      Serial.println("Full Open");
      actionStarted = millis();
      status = 3;
      stopCurtain();
      delay(500);
      openCurtain();
      delay(2000);
    }
  } else if (status == 2) {
    // CLOSING
    if (digitalRead(MANUAL_2) == HIGH) {
      status = 0;
      stopCurtain();
    } else if ((millis() - actionStarted) > FULL_CONTROL_TIME) {
      Serial.println("Full Close");
      actionStarted = millis();
      status = 4;
      stopCurtain();
      delay(500);
      closeCurtain();
      delay(2000);
    }
  } else if (status == 3) {
    // FULL OPEN
    if (digitalRead(MANUAL_1) == LOW || digitalRead(MANUAL_2) == LOW) {
      status = 0;
      stopCurtain();
      delay(1000);
    } else if ((millis() - actionStarted) > FULL_ACTION_TIME) {
      status = 0;
      stopCurtain();
    }
  } else if (status == 4) {
    // FULL CLOSE
    if (digitalRead(MANUAL_1) == LOW || digitalRead(MANUAL_2) == LOW) {
      status = 0;
      stopCurtain();
      delay(1000);
    } else if ((millis() - actionStarted) > FULL_ACTION_TIME) {
      status = 0;
      stopCurtain();
    }
  } else if (status == 11) {
    // OPENING
    if (digitalRead(EXTERNAL_1) == (EXTERNAL_INVERT ? LOW : HIGH) || (millis() - actionStarted) > FULL_ACTION_TIME) {
      status = 0;
      stopCurtain();
    }
  } else if (status == 12) {
    // OPENING
    if (digitalRead(EXTERNAL_2) == (EXTERNAL_INVERT ? LOW : HIGH) || (millis() - actionStarted) > FULL_ACTION_TIME) {
      status = 0;
      stopCurtain();
    }
  }
  delay(20);
}