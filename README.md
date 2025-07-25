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
 
### [gui](gui)
Modular LVGL layout

---

Inspired by / credit to:  
https://github.com/RyanEwen/esphome-lvgl  
https://github.com/agillis/esphome-modular-lvgl-buttons