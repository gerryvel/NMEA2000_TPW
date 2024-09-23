# TPW Sensor Modul

Idea and basics for this Pojekt is "NMEA2000-TempPressure" from [@AK-Homberger](https://github.com/AK-Homberger) and "NMEA0183-to-NMEA2000" with ESP32 [@AndrasSzep](https://github.com/AndrasSzep).

This repository shows how to measure temperature and barometric pressure with a BMP388 sensor and send it to NMEA2000 network.
Additional work a Gateway with inputs NMEA0183 TCP-Stream from my Windsensor "NoWa" or "Yachta" [@norbert-walter](https://github.com/norbert-walter) and send to NMEA2000.

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

![grafik](https://github.com/user-attachments/assets/ed22c02f-b8a3-47e3-8b4a-da89745346ac)


# PCB

Aisler PCB Layout [link](https://aisler.net/p/NZFHAMAJ)

# Hardware

![grafik](https://github.com/user-attachments/assets/eecf0db0-d7b9-4051-9207-430e125e7a3d)


# Partlist:

- Adafruit Huzzah! ESP32 (for programming need USB-Adapter)[Link](https://www.exp-tech.de/plattformen/internet-of-things-iot/9350/adafruit-huzzah32-esp32-breakout-board)
- SN65HVD230 [Link](https://www.reichelt.de/high-speed-can-transceiver-1-mbit-s-3-3-v-so-8-sn-65hvd230d-p58427.html?&trstct=pos_0&nbc=1)
- Traco-Power TSR 1-2450 for 12V / 5V [Link](https://www.reichelt.de/dc-dc-wandler-tsr-1-1-w-5-v-1000-ma-sil-to-220-tsr-1-2450-p116850.html?search=tsr+1-24)
- P4SMAJ26CA, SMA Bidirectional TVS Diode, 26V, 400W
- PESD5V0L2BT, Dual bidirectional TVS diode
- B360, SMD-Diode
- EPCO B82789C0513, EMI 2-inductor filter
- Case Wago 789

# Website

![grafik](https://github.com/gerryvel/NMEA2000_TPW/assets/17195231/3d27c48d-9003-4b28-b112-30b1559b3bdd)

![grafik](https://github.com/gerryvel/NMEA2000_TPW/assets/17195231/cf0d31f0-2dc2-4f43-9a57-4670f1886f26)

![grafik](https://github.com/gerryvel/NMEA2000_TPW/assets/17195231/f34b5f2e-7c5c-434d-bbc7-f006c7c9dbe9)

# Plotter

![image](https://github.com/gerryvel/NMEA2000_TPW/blob/89836d41f83f9eaae73e8502d0ef879308bd933f/4DEDE642-D2EE-429E-9A56-A173FFFC7A6C.jpeg)

# Versions

- 2.3 Update Windvalues for N2k (0-180 / 0- -180)
- 2.2 Update read values from windsensor, Update PCB
- 2.1 Update Website Gauge's
- 2.0 Comlete Website update, Windsensor Stream read update (code and html files)
- 1.3 Update Windsensor Wlan connect code
- 1.2 Update Website
- 1.1 Update PCB
- 1.0 working Version
