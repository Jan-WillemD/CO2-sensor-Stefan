# CO2-sensor-Stefan

5V - CO2 (rood) en LED (rood)
GND - CO2 (zwart) en  LED (zwart)  - DHT11 - (blauw)

D2: neopixel (groen) comms

CO2 (UART)
D0: Co2 groen (kant van sensor, RX)
D4: CO2 (geel- TX)

cO2: (PPM)
D3: co2 groen (kant van voeding) PPM

D1: DHT11 (wit) - data
3v3: dht + (oranje)
Pullup WEERSTAND tussen DATA en +

# install for filesystem:
esp8266 library

install esp8266fs-plugin: https://github.com/esp8266/arduino-esp8266fs-plugin/releases
download, unzip and put jar file in tool folder in arduino directory





