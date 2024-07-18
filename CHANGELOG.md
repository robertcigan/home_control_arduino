## Home Control Arduino v9 (2023-12)

* Curtain device with open and close pin added
* milis() overflow correctly fixed

## Home Control Arduino v8 (2022-03)

* Relay inverted option support

## Home Control Arduino v7 (2021-12)

* Added PWM output device

## Home Control Arduino v6 (2021-07)

* Connection timeout 20s added because network client is not detecting connection properly

## Home Control Arduino v5 (2021-06)

* Poll for input devices is set via add command (sofware set, not hardcoded)
* Switch and Button login input can be inverted via add command

## Home Control Arduino v4 (2021-05)

* ESP8266 support
* Better network reset/handling

## Home Control Arduino v3 (2021-04)

* Remember devices setup once, do not ask for devices setup on connection if devices are already setup
* Added "send_devices" output command to ask for devices setup (pin assignment)
* Round analog input output format to 2 decimals
* Serial output speed changed to 115200bd
* Conditionaly show memory debug info to Serial output - SHOW_MEMORY_IN_SERIAL
* Conditionaly show pin values changes to Serial output - SHOW_VALUES_IN_SERIAL

## Home Control Arduino v2 (2021-02)

* Use JSON format insted of custom communication format
* Use EEPROM to store network settings (analog pins A0-A4 are now free to use)