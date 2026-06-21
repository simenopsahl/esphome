# ESPHome Config
 
Split configuration for ESPHome.  
Mostly M5Stack products

### [board](board)
Base ESP32 model variant functions  
M5Stack: AirQ, ATOM S3 Lite, ATOM S3R, CoreInk and more  
WaveShare ePaper driver board
 
### [board/pins](board/pins)
PINout for board variants

### [board/extension](board/extension)
i2c, SPI, UART & GPIO  
HAT, Grove and breakout extension functions  
M5Stack: ATOM Mate HAT, ATOM Port ABC, Unit pA Hub and more
 
### [board/module](board/module)
Sensors and peripherals  
M5Stack: HAT ENV.III, Unit ENV.IV, Unit PIR  
ePaper basic functions and scripts

### [bluetooth](bluetooth)
Bluetooth proxy and BLE scanner/tracker packages

### [common](common)
Shared base configuration included in all ESPHome projects  
WiFi, API, OTA, logging, time and restart defaults

### [features](features)
Reusable feature packages for cross-device functionality  
Combined sensors, IR codes, NordPool alerts, status LEDs

### [gui](gui)
Modular LVGL layout with resolution-specific configs, themes, pages and widgets

---

Inspired by / credit to:  
https://github.com/RyanEwen/esphome-lvgl  
https://github.com/agillis/esphome-modular-lvgl-buttons