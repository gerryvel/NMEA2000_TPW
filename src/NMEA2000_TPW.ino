/*
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  NMEA2000 Temperature and Barometric Pressure with BMP280 or 388.
  Reads messages from NMEA0183 WindSensor and forwards them to the N2k bus.

  V2.5 vom 05.02.2025, gerryvel Gerry Sebb
*/

#include <Arduino.h>
#include "BoardInfo.h"
#include "configuration.h"
#include "helper.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_BMP280.h>
#include <wire.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LED.h"
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> 
#include "web.h"

// N2k
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>

// NMEA0183
#include "NMEA0183.h"
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include <NMEA0183Stream.h>

// Can Bus
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object

// BMP
Adafruit_BMP280 bmp280;
Adafruit_BMP3XX bmp3xx;

static ETSTimer intervalTimer;

/***************************** NMEA2000 **************************/

String NMEA_Info; // formatted Output NMEA2000-Message
int NodeAddress = 0; // To store last Node Address
Preferences preferences; // Nonvolatile storage on ESP32 - To store LastDeviceAddress

// Set the information for other bus devices, which messages we support
const unsigned long TransmitMessages[] PROGMEM = { 130310L, // Outside Environmental parameters
                                                   130316L, // Temperature
                                                   130314L, // Pressure
                                                   130306L, // Wind
                                                   0
                                                 }; 



bool IsTimeToUpdate(unsigned long NextUpdate){
  return (NextUpdate < millis());
}
unsigned long InitNextUpdate(unsigned long Period, unsigned long Offset = 0){
  return millis() + Period + Offset;
}

void SetNextUpdate(unsigned long & NextUpdate, unsigned long Period){
  while (NextUpdate < millis()) NextUpdate += Period;
}

void CheckSourceAddressChange() {
  int SourceAddress = NMEA2000.GetN2kSource();

  if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory 
    NodeAddress = SourceAddress;      // Set new Node Address (to save only once)
    preferences.begin("nvs", false);
    preferences.putInt("LastNodeAddress", SourceAddress);
    preferences.end();
    Serial.printf("Address Change: New Address=%d\n", SourceAddress);
  }
}

void SendN2kWind(void){
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, WindSendOffset);
  tN2kMsg N2kMsg;

  if (IsTimeToUpdate(SlowDataUpdated)){
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);      
    Serial.print("N2k Wind Data: ");
    Serial.printf("Windrichtung: %f ° - Windgeschw: %f m/s, Referenz: %i\n", dMWV_WindAngle, dMWV_WindSpeed, dMWV_Reference);

    SetN2kPGN130306(N2kMsg, 0, dMWV_WindSpeed, dMWV_WindAngle, tN2kWindReference(dMWV_Reference)); // WindReference: AWA, AWS 
    NMEA2000.SendMsg(N2kMsg);
    Serial.printf("%s\nData: %s, PGN: %i, Priority: %i, SourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
  }
}

void SendN2kCabinTemperatur(void){
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, Temp1SendOffset);
  tN2kMsg N2kMsg;

  if (IsTimeToUpdate(SlowDataUpdated)){
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);
    Serial.print("N2k Temperatur Data: ");
    Serial.printf("Temperatur: %3.1f °C\n", fbmp_temperature);  // Cabin temperature

    SetN2kPGN130316(N2kMsg, 0, 0, N2kts_MainCabinTemperature, CToKelvin(fbmp_temperature), N2kDoubleNA);
    NMEA2000.SendMsg(N2kMsg);  
    Serial.printf("%s\nData: %s, PGN: %i, Priority: %i, SourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
  }
}

void SendN2kOutdoorTemperatur(void){
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, Temp2SendOffset);
  tN2kMsg N2kMsg;

  if (IsTimeToUpdate(SlowDataUpdated)){
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);
    Serial.print("N2k Temperatur Data: ");
    Serial.printf("Temperatur: %3.1f °C\n", fWindSensorTemp);  // Outdoor temperature

    SetN2kPGN130316(N2kMsg, 0, 0, N2kts_OutsideTemperature, CToKelvin(fWindSensorTemp), N2kDoubleNA);
    NMEA2000.SendMsg(N2kMsg);  
    Serial.printf("%s\nData: %s, PGN: %i, Priority: %i, SourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
  }
}

void SendN2kPressure(void){
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, PressSendOffset);
  tN2kMsg N2kMsg;

  if (IsTimeToUpdate(SlowDataUpdated)){
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);
    Serial.print("N2k Pressure & Altitude Data: ");
    Serial.printf("Luftdruck: %3.2f hPa - Hoehe: %3.0f m\n", fbmp_pressure/100, fbmp_altitude);

    SetN2kPGN130314(N2kMsg, 0, 0, N2kps_Atmospheric, (fbmp_pressure));
    NMEA2000.SendMsg(N2kMsg);
    Serial.printf("%s\nData: %s, PGN: %i, Priority: %i, SourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
  }
}

//================================== SETUP ==============================//
void setup()
{
  Serial.begin(115200);

  Serial.printf("TPW Sensor setup %s start\n", Version);

//Filesystem
	if (!LittleFS.begin(true)) {
		Serial.println("An Error has occurred while mounting LittleFS");
		return;
	}
	Serial.println("Memory LittleFS used:");
	Serial.println(LittleFS.usedBytes());

	File root = LittleFS.open("/");
  listDir(LittleFS, "/", 3);
  readConfig("/config.json");
	CL_SSID = tWeb_Config.wAP_SSID;
	CL_PASSWORD = tWeb_Config.wAP_Password;
  // Sensortyp = tWeb_Config.wBMP_Sensortype;
  Serial.println("Configdata WIFI-Client:\n Client SSID: " + CL_SSID + " , Passwort: " + CL_PASSWORD + " , Sensortyp: " + Sensortyp);

// I2C
  Wire.begin(BMP_SDA, BMP_SCL);
  I2C_scan();
//LED
  LEDInit();
// Boardinfo	
  sBoardInfo = boardInfo.ShowChipIDtoString();

//Wifi
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPdisconnect();
  if(WiFi.softAP(AP_SSID, AP_PASSWORD, channel, hide_SSID, max_connection)){
    WiFi.softAPConfig(IP, Gateway, NMask);
    Serial.println("");
    Serial.println("Network " + String(AP_SSID) + " running");
    LEDon(LED(Green));
    delay(1000);
  } else {
      Serial.println("Starting AP failed.");
      LEDoff(LED(Green));
      LEDon(LED(Red));  
      delay(1000); 
      ESP.restart();
  }
  
  WiFi.hostname(HostName);
  Serial.println("Set Hostname done");

WiFiDiag();

// Anmelden mit WiFi als Client an Windmesser
Serial.println("Client Connection");
  WiFi.disconnect(true);
  delay(1000);   
  int count = 0; // Init Counter WFIConnect  

WiFi.begin((const char*)CL_SSID.c_str(), (const char*)CL_PASSWORD.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    LEDon(LED(Blue));
    delay(500);
    Serial.print(".");
    LEDflash(LED(Red));
   count++;
    if (count = 10) break;
  }
  if (WiFi.isConnected()) {
   bConnect_CL = 1;
    Serial.println("Client Connected");
  }
  else
    Serial.println("Client Connection failed");
    LEDoff(LED(Blue));
    WiFi.reconnect(); 

// Start OTA
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

// Start TCP (HTTP) server
	server.begin();
	Serial.println("TCP server started");

// Mit dem NMEA-Server verbinden
  if (nmeaclient.connect(SERVER_HOST_NAME, TCP_PORT)) {
    Serial.println("Connection to NMEA-Server ready");
  } else {
    Serial.println("Connection to NMEA-Server failure");
  }

// Start NMEA0183 stream handling
  NMEA0183.SetMessageStream(&nmeaclient);
  NMEA0183.Open();
  Serial.println("NMEA0183 started");

// BMP begin & setup
  if (Sensortyp == 0){
      sBMP = "BMP280";
    if (!bmp280.begin()) 
  {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
  }
  } else {
      sBMP = "BMP388";
    if (!bmp3xx.begin_I2C()) 
  {
    Serial.println("Could not find a valid BMP3xx sensor, check wiring!");
  }
  bmp3xx.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp3xx.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp3xx.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp3xx.setOutputDataRate(BMP3_ODR_50_HZ); 
  }



// MDNS
  MDNSResponder mdns;
  MDNS.begin(HostName);
  MDNS.addService("http", "tcp", 80);

  website();

  //***********************************************Setup NMEA2000 system*******************************************
  // Reserve enough buffer for sending all messages. This does not work on small memory devices like Uno or Mega
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  NMEA2000.SetProductInformation("TPW 01", // Manufacturer's Model serial code
                                 107, // Manufacturer's product code
                                 "TPW Sensor Module",  // Manufacturer's Model ID
                                 "2.2.0.0 (2024-08-22)",  // Manufacturer's Software version code
                                 "2.0.0.0 (2023-01-26)" // Manufacturer's Model version
                                );
  // Set device information
  NMEA2000.SetDeviceInformation(8, // Unique number. Use e.g. Serial number.
                                130, // Device function=PC Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                25, // Device class=Inter/Intranetwork Device. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                               );

  // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text. Leave uncommented for default Actisense format.

  preferences.begin("nvs", false);                          // Open nonvolatile storage (nvs)
  NodeAddress = preferences.getInt("LastNodeAddress", 34);  // Read stored last NodeAddress, default 34
  preferences.end();
  Serial.printf("NodeAddress=%d\n", NodeAddress);

  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, NodeAddress);
  NMEA2000.ExtendTransmitMessages(TransmitMessages);

  Serial.println(" opening NMEA2000");
  if (NMEA2000.Open())
    Serial.println(" NMEA2000 Initialized");
  else
    Serial.println(" NMEA2000 Initialized failed");
  
  // Setup LED off
  LEDoff_RGB();
}
//=================================== Loop ==============================//
void loop()
{ 
  // Listen NMEA0183 or reconnect Wifi
  bConnect_CL = WiFi.status() == WL_CONNECTED ? 1 : 0;
  Serial.printf("WLAN Status: %s \n", sWifiStatus(WiFi.status()));
  Serial.printf("bConnect Client: %i \n", bConnect_CL);
  { // Listen NMEA0183
    if (bConnect_CL == 1){ // Connected an listen
      Serial.printf("Wifi %s connencted!\n", CL_SSID);
      flashLED(LED(Green), 5);
      NMEA0183_ParseMessages();
      NMEA0183_read();
      delay(100);
    }
    else{
      Serial.println("Wifi connect failed!\n");
      bConnect_CL = 0;
      Serial.printf("Reconnecting to %s\n", CL_SSID);
      LEDoff(LED(Green));
      WiFi.reconnect();    // wifi down, reconnect here
      delay(500);
      int WLcount = 0;
     int UpCount = 0;
      while (WiFi.status() != WL_CONNECTED && WLcount < 50){
        delay(50);
        Serial.printf(".");
        //LEDflash(LED(Red));
        flashLED(LED(Red), 5);
        if (UpCount >= 20)  // just keep terminal from scrolling sideways
        {
          UpCount = 0;
          Serial.printf("\n");
        }
        ++UpCount;
        ++WLcount;
      }
    }
  }

  NMEA0183_reconnect();
  
ArduinoOTA.handle();

// Status AP 
  Serial.printf("Soft-AP IP address: %s\n", WiFi.softAPIP().toString());
  sCL_Status = sWifiStatus(WiFi.status());

 delay(500);

 //Read BMP  
  if (Sensortyp == 0)
  {
    fbmp_temperature = bmp280.readTemperature();
    fbmp_pressure = bmp280.readPressure();
    sBMP_Status = "BMP lesen erfolgreich";
    flashLED(LED(Blue), 5);
  } else {
    if (!bmp3xx.performReading()) 
    {       
      Serial.println("BMP reading error");
      sBMP_Status = "BMP Lesefehler";
      LEDflash(LED(Blue));
    } else {
      Serial.println("BMP read successful");
      sBMP_Status = "BMP lesen erfolgreich";
      fbmp_pressure = bmp3xx.readPressure();
      fbmp_temperature = bmp3xx.readTemperature();
      fbmp_altitude = bmp3xx.readAltitude(SEALEVELPRESSURE_HPA);
      // LEDblink(LED(Blue));
      flashLED(LED(Blue), 5);
      delay(100);
    }
  }

  //N2K
    SendN2kWind();
    SendN2kPressure();
    SendN2kCabinTemperatur();
    SendN2kOutdoorTemperatur();
    NMEA2000.ParseMessages();
    CheckSourceAddressChange();

    freeHeapSpace();

if (IsRebootRequired) {
		Serial.println("Rebooting ESP32: "); 
		delay(1000); // give time for reboot page to load
		ESP.restart();
		}
}
