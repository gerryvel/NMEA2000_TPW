# NMEA2000_TPW Version 2.0

Basics for this Pojekt is "NMEA2000-TempPressure" from @HOMBERGER and "NMEA0183-to-NMEA2000" with ESP32 @AndrasSzep.

This repository shows how to measure temperature and barometric pressure with a BMP388 sensor and send it to NMEA2000 network.
Additional work a Gateway with inputs NMEA0183 TCP-Stream from my Windsensor "NoWa" or "Yachta" @norbert-walter and send to NMEA2000.

The data is sent to NMEA2000 network with PGN130310 and PGN130306 (Outside Environmental Parameters and Wind).

The project requires the NMEA2000 and the NMEA2000_esp32 libraries from Timo Lappalainen: https://github.com/ttlappalainen. Both libraries have to be downloaded and installed.

For the BMP388 the Adafruit BMP3xx library has to be installed via the library manager.
For HTML-Files use LittleFS Filesystem, you find her in /data directory.

The ESP32 in this project is an Adafruit Huzzah! ESP32. This is a small module without USB connector.

For the ESP32 CAN bus, i used the SN65HVD230 chip as transceiver. The correct GPIO ports are defined in the main sketch. For this project, I use the pins GPIO4 for CAN RX and GPIO5 for CAN TX.

The 12 Volt is reduced to 5 Volt with a DC Step-Down_Converter. 12V DC comes from the N2k Bus Connector with the M12 Connector.

- Adafruit Huzzah! ESP32 (for programming need USB-Adapter)
- Traco-Power TSR 1-2450 for 12V / 5V
- RGB LED Kingbright L-154A4SURKQBDZGW
- TI SMD-Chip SN65HVD230
- Case Wago 789-905

# Wiring diagram

[SchaltplanTPW_kiCad.pdf](https://github.com/gerryvel/NMEA2000_TPW/files/11401880/SchaltplanTPW_kiCad.pdf)

# PCB

Aisler PCB Layout https://aisler.net/p/NZFHAMAJ

# Hardware

![PNG-Bild](https://github.com/gerryvel/NMEA2000_TPW/assets/17195231/3f11f60e-832c-4a33-9e59-35a974e494bd)
![_com apple Pasteboard dirmnK](https://github.com/gerryvel/NMEA2000_TPW/assets/17195231/51dbb481-9931-4788-9c1b-f460ad98ce15)

# Partlist:

- Adafruit Huzzah! ESP32 (for programming need USB-Adapter)[Link](https://www.exp-tech.de/plattformen/internet-of-things-iot/9350/adafruit-huzzah32-esp32-breakout-board)
- SN65HVD230 [Link](https://www.reichelt.de/high-speed-can-transceiver-1-mbit-s-3-3-v-so-8-sn-65hvd230d-p58427.html?&trstct=pos_0&nbc=1)
- Traco-Power TSR 1-2450 for 12V / 5V [Link](https://www.reichelt.de/dc-dc-wandler-tsr-1-1-w-5-v-1000-ma-sil-to-220-tsr-1-2450-p116850.html?search=tsr+1-24)
- Case Wago 789

# Website

![image](https://user-images.githubusercontent.com/17195231/228060438-5684a891-3952-4b01-8499-b57e9f5e0bac.jpeg)

![image](https://user-images.githubusercontent.com/17195231/228060533-b21ea270-33f6-4de0-9d59-917f8d27dbc2.jpeg)

![image](https://user-images.githubusercontent.com/17195231/228060619-1ddb1600-8da0-468c-b6b3-252db2e3b783.jpeg)

# Plotter

![image](https://github.com/gerryvel/NMEA2000_TPW/blob/89836d41f83f9eaae73e8502d0ef879308bd933f/4DEDE642-D2EE-429E-9A56-A173FFFC7A6C.jpeg)

