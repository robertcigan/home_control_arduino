## Home Control Arduino v3 (2021-04) ##

* Remember devices setup once, do not ask for devices setup on connection if devices are already setup
* Added "send_devices" output command to ask for devices setup (pin assignment)
* Round analog input output format to 2 decimals
* Serial output speed changed to 115200bd
* Conditionaly show memory debug info to Serial output - SHOW_MEMORY_IN_SERIAL
* Conditionaly show pin values changes to Serial output - SHOW_VALUES_IN_SERIAL

## Home Control Arduino v2 (2021-02) ##

* Use JSON format insted of custom communication format
* Use EEPROM to store network settings (analog pins A0-A4 are now free to use)