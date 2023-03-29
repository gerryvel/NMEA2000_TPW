# NMEA2000_TPW Version 1.3

Basics for this Pojekt is "NMEA2000-TempPressure" from @HOMBERGER and "NMEA0183-to-NMEA2000" with ESP32 @AndrasSzep.

This repository shows how to measure temperature and barometric pressure with a BMP388 sensor and send it to NMEA2000 network.
Additional work a Gateway with inputs NMEA0183 TCP-Stream from my Windsensor "NoWa" and Output to same NMEA2000.

The data is sent to NMEA2000 network with PGN130310 and PGN130306 (Outside Environmental Parameters and Wind).

The project requires the NMEA2000 and the NMEA2000_esp32 libraries from Timo Lappalainen: https://github.com/ttlappalainen. Both libraries have to be downloaded and installed.
Also you need the Async TCP Library for ESP32 Arduino https://github.com/me-no-dev/AsyncTCP and the changed NMEA0183 äLibrary and BoatData (not the Original @Lappalainen!) @Andras Szep.

For the BMP388 the Adafruit BMP3xx library has to be installed via the library manager.
For HTML-Files use LittleFS Filesystem, you find her in /data directory.

The ESP32 in this project is an Adafruit Huzzah! ESP32. This is a small module without USB connector.

For the ESP32 CAN bus, i used the "Waveshare SN65HVD230 Can Board" as transceiver. For small space i cut the connectors and mount this board upside down. The correct GPIO ports are defined in the main sketch. For this project, I use the pins GPIO4 for CAN RX and GPIO5 for CAN TX.

The 12 Volt is reduced to 5 Volt with a DC Step-Down_Converter. 12V DC comes from the N2k Bus Connector with the M12 Connector.

- Adafruit Huzzah! ESP32 (for programming need USB-Adapter)
- Traco-Power TSR 1-2450 for 12V / 5V
- RGB LED Kingbright L-154A4SURKQBDZGW
- Waveshare SN65HVD230 Can Board (adapted, cutted the (also without) connector)
- Case Wago 789

# Schaltplan

![grafik](https://user-images.githubusercontent.com/17195231/228666974-7ecb2b85-ea2b-4d19-b73a-715880c24c6a.png)

# Hardware

![TPW N2k](https://user-images.githubusercontent.com/17195231/201548865-527490c7-9898-4cfb-8c67-161541537aac.jpg)

![grafik](https://user-images.githubusercontent.com/17195231/227721635-994e5d76-131b-49b2-9e4c-9c372bde4454.png)

![image](https://user-images.githubusercontent.com/17195231/227989873-d6256e00-5c0f-4283-a65b-ce08e13113d3.jpeg)

![image](https://user-images.githubusercontent.com/17195231/228036198-5dd91f80-0a2a-475a-9819-7a0e74eb7e1d.jpeg)

# Website

![image](https://user-images.githubusercontent.com/17195231/228060438-5684a891-3952-4b01-8499-b57e9f5e0bac.jpeg)

![image](https://user-images.githubusercontent.com/17195231/228060533-b21ea270-33f6-4de0-9d59-917f8d27dbc2.jpeg)

![image](https://user-images.githubusercontent.com/17195231/228060619-1ddb1600-8da0-468c-b6b3-252db2e3b783.jpeg)

# Plotter

![image](https://github.com/gerryvel/NMEA2000_TPW/blob/89836d41f83f9eaae73e8502d0ef879308bd933f/4DEDE642-D2EE-429E-9A56-A173FFFC7A6C.jpeg)


