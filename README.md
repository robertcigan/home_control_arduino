# home_control_arduino


TODO: add wifi setting support to EEPROM storage

supported devices / commands through json

add:
  type: "switch"
  id: 
  pin:
  poll:
  inverted:

add:
  type: "button"
  id:
  pin:.
  poll:
  inverted:

add:
  type: "relay"
  id:
  pin:

# connected via Serial1 port on MMega2560 (pin 18 + 19)
add:
  type: "player"
  id: 

add:
  type: "distance"
  id:
  write_pin:
  read_pin:
  poll:

add:
  type: "analog_input"
  id:
  apin:
  poll:

ping: true

responds with { pong: true, ssid: "xxx", signal_strength: 55, version: 4 }



  

  Board configuraton over serial:
  use mac address prefix DE:AD
  ie. DE:AD:97:8A:30:60


