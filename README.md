# Abraxas_TP

Basic for this Pojekt is NMEA2000-TempPressure from @HOMBERGER and AndrasSzep / NMEA0183-to-NMEA2000 with ESP32.

This repository shows how to measure temperature and barometric pressure with a BMP280 sensor and send it to NMEA2000 network.
Additional work a Gateway with inputs NMEA0183 TCP-Stream and Output to NMEA2000.



The data is sent to NMEA2000 network with PGN130310 and PGN130306 (Outside Environmental Parameters and Wind)

The project requires the NMEA2000 and the NMEA2000_esp32 libraries from Timo Lappalainen: https://github.com/ttlappalainen. Both libraries have to be downloaded and installed.

For the BMP280 the Adafruit BMP280 library has to be installed via the library manager.

The ESP32 in this project is an ESP32 NODE MCU from AzDelivery. Pin layout for other ESP32 devices might differ.

For the ESP32 CAN bus, I used the "Waveshare SN65HVD230 Can Board" as transceiver. It works well with the ESP32. The correct GPIO ports are defined in the main sketch. For this project, I use the pins GPIO4 for CAN RX and GPIO5 for CAN TX.

The 12 Volt is reduced to 5 Volt with a DC Step-Down_Converter.
