# Weather Station Control System

This is an Arduino-based weather station control system that gathers data from various sensors including temperature, humidity, pressure, and wind. It utilizes the ESP32 microcontroller to communicate with the sensors and transmit the data over WiFi. The system is designed to log the data to an SD card and send it to a remote server via HTTP for further analysis.
This is not a ready made sollutions, you will need to tinker with this code.

## Features

- Collects data from AHT21 temperature/humidity sensor and BMP-280 pressure sensor.
- Monitors wind speed and direction using a wind sensor connected via the MAX485 interface.
- Detects rainfall using an attached rain sensor.
- Supports OTA (Over-The-Air) updates for firmware updates and improvements.

## Hardware Requirements

- ESP32 microcontroller
- AHT21 temperature/humidity sensor
- BMP-280 pressure and temperature sensor
- Wind sensor with MAX485 interface
- Rain sensor
- SD card module

## Setup

1. Connect the sensors and modules to the appropriate pins on the ESP32 microcontroller.
2. Upload the provided Arduino sketch to the ESP32 using the Arduino IDE.
3. Ensure proper power supply and connectivity for the ESP32 and the connected sensors.
4. Monitor the serial output for debugging and operational information.

## Dependencies

- Wire.h
- uRTCLib.h
- SD.h
- SPI.h
- SoftwareSerial.h
- Adafruit_AHTX0.h
- Adafruit_BMP280.h
- WiFi.h
- HTTPClient.h
- WiFiUdp.h
- NTPClient.h
- TimeLib.h
- ESPmDNS.h
- ArduinoOTA.h

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
