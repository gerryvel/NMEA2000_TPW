# Temperature, pressure and wind Data > NMEA2000

![ESP32](https://img.shields.io/badge/ESP32-grey?logo=Espressif)
![KiCad](https://img.shields.io/badge/KiCad-darkblue?logo=KiCad)
![Relaise](https://img.shields.io/github/release-date/gerryvel/NMEA2000_TPW?)
![lastcommit](https://img.shields.io/github/last-commit/gerryvel/NMEA2000_TPW)
![OBP](https://img.shields.io/badge/Sailing_with-OpenBoatsProjects-blue)
[![OPB](https://img.shields.io/badge/Sailing with-OpenBoatsProjects-blue)](link=https://open-boat-projects.org/de/)
https://img.shields.io/badge/Sailing%20with?labelColor=Blue&link=https%3A%2F%2Fopen-boat-projects.org%2Fde%2F


## Description
This repository shows how to measure the 
- temperature
- barometric pressure
- Windspeed
- Winddirection

and send it as NNMEA2000 meassage.
- PGN 130306 // Wind
- PGN 130314 // Pressure
- PGN 130316 // Temperature 

In addition, all data and part of the configuration are displayed as a website.

## Based on the work of

Idea and basics for this Pojekt is "NMEA2000-TempPressure" from [@AK-Homberger](https://github.com/AK-Homberger).

This repository shows how to measure  and  with a BMP280 or BMP388 sensor and send it to NMEA2000 network.
Additional work a Gateway with inputs NMEA0183 TCP-Stream from my Windsensor "NoWa" or "Yachta" [@norbert-walter](https://github.com/norbert-walter) and send to NMEA2000.

The project requires the NMEA2000 and the NMEA2000_esp32 libraries from Timo Lappalainen: https://github.com/ttlappalainen. 

This project is part of [OpenBoatProject](https://open-boat-projects.org/)

## TPW Sensor Modul

For the BMP 280 and BMP388 the Adafruit BMP3xx library has to be installed via the library manager.
For HTML-Files use LittleFS Filesystem, you find her in /data directory.

The ESP32 in this project is an Adafruit Huzzah! ESP32. This is a small module without USB connector.

For the ESP32 CAN bus, i used the SN65HVD230 chip as transceiver. The correct GPIO ports are defined in the main sketch. For this project, I use the pins GPIO4 for CAN RX and GPIO5 for CAN TX.

The 12 Volt is reduced to 5 Volt with a DC Step-Down_Converter. 12V DC comes from the N2k Bus Connector with the M12 Connector.

## Wiring diagram

![grafik](https://github.com/user-attachments/assets/cedf11dc-f76e-48c0-939f-1261e12a5e92)

## PCB

Aisler PCB Layout [link](https://aisler.net/p/NZFHAMAJ)

## Hardware

![grafik](https://github.com/user-attachments/assets/eecf0db0-d7b9-4051-9207-430e125e7a3d)


## Partlist:

- Adafruit Huzzah! ESP32 (for programming need USB-Adapter)[Link](https://www.exp-tech.de/plattformen/internet-of-things-iot/9350/adafruit-huzzah32-esp32-breakout-board)
- SN65HVD230 [Link](https://www.reichelt.de/high-speed-can-transceiver-1-mbit-s-3-3-v-so-8-sn-65hvd230d-p58427.html?&trstct=pos_0&nbc=1)
- Traco-Power TSR 1-2450 for 12V / 5V [Link](https://www.reichelt.de/dc-dc-wandler-tsr-1-1-w-5-v-1000-ma-sil-to-220-tsr-1-2450-p116850.html?search=tsr+1-24)
- RGB LED Kingbright [Link](https://www.reichelt.de/led-5-mm-bedrahtet-4-pin-rot-gruen-blau-700-1300-300-mcd-60-kbt-l-154a4surkq-p231040.html?&trstct=pol_0&nbc=1) 
- P4SMAJ26CA, SMA Bidirectional TVS Diode, 26V, 400W ([Link](https://www.reichelt.de/tvs-diode-bidirektional-26-v-400-w-do-214ac-sma-p4smaj26ca-p272871.html)
- PESD1CAN, Dual bidirectional TVS diode [Link](https://www.reichelt.de/can-bus-esd-schutzdiode-tvs-24-v-sot-23-3-pesd-1can-p219293.html?&trstct=pos_0&nbc=1)
- B360, SMD-Diode [Link](https://www.reichelt.de/schottkydiode-60-v-3-a-do-214ab-smc-b-360-f-p95202.html?&trstct=pos_1&nbc=1)
- EPCO B82789C0513, EMI 2-inductor filter [Link](https://www.reichelt.de/smd-power-induktivitaet-1812-51-h-epco-b82789c0513-p245680.html?&trstct=pos_0&nbc=1)
- Wago-Case: [Link](https://www.wago.com/de/zubehoer/gehaeuse-55-mm/p/789-120)

## Website

![grafik](https://github.com/gerryvel/NMEA2000_TPW/assets/17195231/3d27c48d-9003-4b28-b112-30b1559b3bdd)

![grafik](https://github.com/gerryvel/NMEA2000_TPW/assets/17195231/cf0d31f0-2dc2-4f43-9a57-4670f1886f26)

![grafik](https://github.com/gerryvel/NMEA2000_TPW/assets/17195231/f34b5f2e-7c5c-434d-bbc7-f006c7c9dbe9)

## Plotter

![image](https://github.com/gerryvel/NMEA2000_TPW/blob/89836d41f83f9eaae73e8502d0ef879308bd933f/4DEDE642-D2EE-429E-9A56-A173FFFC7A6C.jpeg)

## Versions

- 2.5 Update and add website details
- 2.4 Update windvalues and add windsensortemp for N2k, website cosmetic
- 2.3 Update windvalues for N2k
- 2.2 Update read values from windsensor, Update PCB
- 2.1 Update website Gauge's
- 2.0 Complete Website update, Windsensor Stream read update (code and html files)
- 1.3 Update windsensor Wlan connect code
- 1.2 Update website
- 1.1 Update PCB
- 1.0 working Version
