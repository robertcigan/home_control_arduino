# Home Control Arduino Firmware

[Home Control Arduino Firmware](README.md) | [Changelog](CHANGELOG.md)

[Home Control](https:/github.com/robertcigan/home_control) Arduino FW codebase for flashing Arduino Mega 2650, ESP8266 or EPS32 board. 
Configured to be used within [Platformio in VS Code](https://platformio.org/).

## Initial configuration

After flashing the board you need to configure the board network settings. Network settings is stored in the board EEPROM.

Connect the board via Serial interface to a computer and open the serial console (Arduino or Platformio).  Default bardrate is 115200. Set `server_ip`, `client_ip`,` mac` to set the network settings for ethernet connection. For WiFi connection include also `gateway_ip`, `ssid` and `pass` setting commands to set the network settings. After setting all the parameters, send command `save` to save current settings to EEPROM. If you do not save current settings via `save` command, after resetting the board it will loose all the settings. Once saved reset the board and that's all.

__Available serial interface commands__

* `server_ip=123.123.123.123`
Sets the Home Control server IP address. 

* `client_ip=123.123.123.123`
Sets the board IP address. Make sure it's unique within your network.

* `mac=DE:AD:FF:FF:FF:FF`
Sets the board MAC address. Make sure it's unique within your network. Use prefix DE:AD - ie. DE:AD:97:8A:30:60

* `gateway_ip=123.123.123.123`
Sets the network gateway IP address.

* `ssid=mynetwork`
Sets the WiFi network name to connect to.

* `pass=mypassword`
Password for the WiFi network.

* `save`
Stores current network settings to EEPROM to make it persistent.

## Communication protocol

Communication with the server is on TCP in JSON data format. It has a persisted connection with a duplex communication. Both server and clients (boards) are sending data to each other. Every 10s the server asks via a ping command to receive a pong command from boards to keep the connection alive and detect offline boards. TCP connection timeout does not work reliably unfortunately. 

### Add command

Adds a new device to the board. Device types is listed below. Each device may have different set of options. `id` is required.

`{ "add": { "type: "relay", "id": 7, "default": false, "inverted": false }}` example for relay device

###  Reset devices

Removes all devices from the board.

`{ "reset_devices": true }`

### Ping

Ping command is a request to send  `pong` response to the server with the board status data. Server pings all boards every 10 seconds.

`{ "ping": true }`

### Read

Request to the board to send the device value back to server.

`{ "read": { "id": 5 } }`

### Write

Request to set the specific value to the device based on ID.

`{ "write": { "id": 5, "value": true} }` example for relay device
`{ "write": { "id": 6, "action": "open", "value": 3000} }` example for curtain module - open the curtain for 3000ms

## Status LED

On startup, the LED is turned on and once the connection with the server is established, it's turned off. With each ping command received, the board flashes the internal LED (build-in LED) to indicate proper status of the device. So if the device is working properly, you should see a LED blink every 10s.

## Submodules

### Curtain Switch

In order to support the Curtain Switch module to control reliably devices like curtains, rollers, shutters and etc. you should use this dedicated simple HW module instead of directly controlling 2 relays. It uses a Arduino Nano to control a dual relay module. Best should be wired to Home Control board like Arduino Mega using a optocoupler to ensure any interference or any HW issues due to a long wiring. I'm running several of these modules over Cat5 cable using optocoupler with distances around 10-30m without any problems.

Curtain Switch module enables you to preserve existing manual wall switch to manually control the device with 2 extra inputs thus combining manual switch control and remote control via Home Control automation. In case of any problems with the Home Control system, you can still open/close your curtain/shutters manually like you are used to. 

Using manual switch control, it also adds button hold function so you do not need to hold the control switch all the time to fully open or close the curtain or shutters. When pressing the up/down for 3 seconds (by default), the motor is stopped for half a second indicating to release of the switch/button. If you release it, the module continues to power the motor automatically without a need to hold it. If you keep the switch pressed, it will stop the motor. 

#### Configuring the functionality

Before flashing the curtain module, configure it base on your needs in [modules/curtain_switch/src/main.cpp](https://github.com/robertcigan/home_control_arduino/blob/master/modules/curtain_switch/src/main.cpp). Following are default values.

`#define RELAY_1 2`
First relay arduino output pin that enables the power to the second relay that changes direction of the motor.

`#define RELAY_2 3`
Second relay arduino output pin that changes the direction of the motor.

`#define MANUAL_1 A3`
Up/Open button pin for manual control.

`#define MANUAL_2 A4`
Down/Close button pin for manual control.

`#define EXTERNAL_1 A1`
Home Control 1st pin for automation control.

`#define EXTERNAL_2 A2`
Home Control 2nd pin for automation control.

`#define EXTERNAL_INVERT true`
Invert input logic on automation control. When `false`  it responds to HIGH, when `true` it responds to LOW. By default keep it inverted.

`#define FULL_ACTION_TIME 70e3`
Time in ms to keep the motors running in automated manual control to fully open/close the curtain/shutter.

`#define FULL_CONTROL_TIME 3e3`
Time in ms to engage automated manual control to fully open/close. Ie. hold the button for 3000ms to engage the full open/close mode.


#### Wiring 

_TODO_

## Contributing

I encourage you to contribute to Home Control project! 

## License

Home Control is released under the [MIT License](https://opensource.org/licenses/MIT).
