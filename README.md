# Abraxas_TP

Basics for this Pojekt is "NMEA2000-TempPressure" from @HOMBERGER and "NMEA0183-to-NMEA2000" with ESP32 @AndrasSzep.

This repository shows how to measure temperature and barometric pressure with a BMP280 sensor and send it to NMEA2000 network.
Additional work a Gateway with inputs NMEA0183 TCP-Stream and Output to same NMEA2000.

The data is sent to NMEA2000 network with PGN130310 and PGN130306 (Outside Environmental Parameters and Wind).

The project requires the NMEA2000 and the NMEA2000_esp32 libraries from Timo Lappalainen: https://github.com/ttlappalainen. Both libraries have to be downloaded and installed.
Also you need the Async TCP Library for ESP32 Arduino https://github.com/me-no-dev/AsyncTCP and the changed NMEA0183 Library and BoatData (not the Original @Lappalainen!) @Andras Szep.

For the BMP280 the Adafruit BMP280 library has to be installed via the library manager.

The ESP32 in this project is an ESP32 NODE MCU from JoyIT. Pin layout for other ESP32 devices might differ.

For the ESP32 CAN bus, I used the "Waveshare SN65HVD230 Can Board" as transceiver. It works well with the ESP32. The correct GPIO ports are defined in the main sketch. For this project, I use the pins GPIO4 for CAN RX and GPIO5 for CAN TX.

The 12 Volt is reduced to 5 Volt with a DC Step-Down_Converter (mounted under the ESP). 12V DC comes from the N2k Bus Connector with the M12 Connector.

![Schaltplan PDF](https://github.com/gerryvel/Abraxas_TP/blob/master/ESP32%20Bootselektronik.pdf)

![Schematics](https://github.com/gerryvel/Abraxas_TP/blob/master/photo_2020-10-10_19-02-31.jpg)

![Schematics](https://github.com/gerryvel/Abraxas_TP/blob/master/photo_2020-10-10_19-02-35.jpg)

