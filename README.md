# esp8266-relay
esp8266 1 channel web based relay

this is a one channel remote control with ESP8266 01 wifi module

gpio.0 -----\/\/\/\/\/--- goto base of bc547 with 15k resistor to prevent module goto boot when statup
emmiter of bc547 goto gnd
collector of bc547 goto relay 5v, other side of relay got to +5, and a 1000uf capacitor parllel with relay to remove startup pulses on gpio0

gpio.2 connected to gnd with a key.

if key connected 0.5 sec after boot, module reset to factory setting.
when bord work properly you can toggle relay with key.
if key pressed and user press ON,OFF or Pulse key the setting page appear.

default IP: 194.168.4.1

LGPL
