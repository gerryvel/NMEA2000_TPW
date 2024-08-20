# 1 "C:\\Users\\GERRYA~1\\AppData\\Local\\Temp\\tmp4rgat7rx"
#include <Arduino.h>
# 1 "C:/Users/gerryadmin/Documents/PlatformIO/Projects/NMEA2000_TPW/src/NMEA2000_TPW.ino"
# 25 "C:/Users/gerryadmin/Documents/PlatformIO/Projects/NMEA2000_TPW/src/NMEA2000_TPW.ino"
#include <Arduino.h>
#include "BoardInfo.h"
#include "configuration.h"
#include "helper.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP3XX.h>
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


#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>


#include "NMEA0183.h"
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>


#include <NMEA2000_CAN.h>


Adafruit_BMP3XX bmp;


WiFiClient nmeaclient;
char buffer[128];
int bufferIndex = 0;

static ETSTimer intervalTimer;
void read_nmea0183();
bool IsTimeToUpdate(unsigned long NextUpdate);
void SetNextUpdate(unsigned long & NextUpdate, unsigned long Period);
void CheckSourceAddressChange();
void SendN2kWind(void);
void SendN2kTemperatur(void);
void SendN2kPressure(void);
void setup();
void loop();
#line 66 "C:/Users/gerryadmin/Documents/PlatformIO/Projects/NMEA2000_TPW/src/NMEA2000_TPW.ino"
void read_nmea0183()
{
  if (nmeaclient.connected()) {
    while (nmeaclient.available()) {
      char c = nmeaclient.read();
      Serial.write(c);

      if (c == '\n' || bufferIndex >= sizeof(buffer) - 1) {
        buffer[bufferIndex] = '\0';


        if (strstr(buffer, "VWR") != NULL) {


          float windDirection = 0.0;
          float windSpeed = 0.0;
          sscanf(buffer, "$%*[^,],%f,R,%f,N", &windDirection, &windSpeed);


          Serial.print("Windrichtung: ");
          Serial.println(windDirection);
          Serial.print("Windgeschwindigkeit: ");
          Serial.println(windSpeed);
          dVWR_WindDirectionM = windDirection;
          dVWR_WindSpeedkn = windSpeed;
        }


        bufferIndex = 0;
      } else {
        buffer[bufferIndex++] = c;
      }
    }
  } else {
    Serial.println("Verbindung zum NMEA-Server verloren");
    nmeaclient.stop();


    if (nmeaclient.connect(SERVER_HOST_NAME, TCP_PORT)) {
      Serial.println("Verbunden mit NMEA-Server");
    } else {
      Serial.println("Verbindung zum NMEA-Server fehlgeschlagen");
    }
  }

}

String NMEA_Info;
int NodeAddress = 0;
Preferences preferences;

const unsigned long TransmitMessages[] PROGMEM = { 130310L,
                                                   130312L,
                                                   130314L,
                                                   130306L,
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

  if (SourceAddress != NodeAddress) {
    NodeAddress = SourceAddress;
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
    Serial.printf("VWR Windrichtung: %f ° - VWR Windgeschw: %f kn\n", dVWR_WindDirectionM, dVWR_WindSpeedkn);

    SetN2kPGN130306(N2kMsg, 0, dVWR_WindSpeedkn, DegToRad(dVWR_WindDirectionM), tN2kWindReference::N2kWind_Magnetic);
    NMEA2000.SendMsg(N2kMsg);
    Serial.print("N2k sende Wind: ");
    Serial.printf("%s\nData: %s\nPGN: %i\nPriority: %i\nSourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
  }
}

void SendN2kTemperatur(void){
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, TempSendOffset);
  tN2kMsg N2kMsg;

  if (IsTimeToUpdate(SlowDataUpdated)){
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);
    Serial.print("N2k Temperatur Data: ");
    Serial.printf("Temperatur: %3.1f °C\n", fbmp_temperature);

    SetN2kPGN130312(N2kMsg, 0, 0, N2kts_MainCabinTemperature, CToKelvin(fbmp_temperature), N2kDoubleNA);
    NMEA2000.SendMsg(N2kMsg);
    Serial.printf("%s\nData: %s\nPGN: %i\nPriority: %i\nSourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
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
    Serial.print("N2k sende Pressure & Altitude: ");
    Serial.printf("%s\nData: %s\nPGN: %i\nPriority: %i\nSourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
  }
}


void setup()
{
  Serial.begin(115200);

  Serial.printf("TPW Sensor setup %s start\n", Version);


 if (!LittleFS.begin(true)) {
  Serial.println("An Error has occurred while mounting LittleFS");
  return;
 }
 Serial.println("Speicher LittleFS benutzt:");
 Serial.println(LittleFS.usedBytes());
 File root = LittleFS.open("/");
  listDir(LittleFS, "/", 3);
  readConfig("/config.json");
 CL_SSID = tAP_Config.wAP_SSID;
 CL_PASSWORD = tAP_Config.wAP_Password;
  Serial.println("Client SSID: " + CL_SSID + " , Passwort: " + CL_PASSWORD);

  freeHeapSpace();


  Wire.begin(BMP_SDA, BMP_SCL);
  I2C_scan();

  LEDInit();
  LEDoff();

  sBoardInfo = boardInfo.ShowChipIDtoString();


  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPdisconnect();
  if(WiFi.softAP(AP_SSID, AP_PASSWORD, channel, hide_SSID, max_connection)){
    WiFi.softAPConfig(IP, Gateway, NMask);
    Serial.println("");
    Serial.println("Network " + String(AP_SSID) + " running");

    delay(1000);
  } else {
      Serial.println("Starting AP failed.");
      digitalWrite(LED(Red), HIGH);
      delay(1000);
      ESP.restart();
  }

  WiFi.hostname(HostName);
  Serial.println("Set Hostname done");

WiFiDiag();


Serial.println("Client Connection");
  WiFi.disconnect(true);
  delay(1000);
  int count = 0;
WiFi.begin(CL_SSID, CL_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    LEDflash(LED(Red));
   count++;
    if (count = 10) break;
  }
  if (WiFi.isConnected()) {
   bConnect_CL = 1;
    Serial.println("Client Connected");
    LEDflash(LED(Green));
  }
  else
    Serial.println("Client Connection failed");
    WiFi.reconnect();


  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else
        type = "filesystem";


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


 server.begin();
 Serial.println("TCP server started");


  if (nmeaclient.connect(SERVER_HOST_NAME, TCP_PORT)) {
    Serial.println("Verbunden mit NMEA-Server");
  } else {
    Serial.println("Verbindung zum NMEA-Server fehlgeschlagen");
  }


  if (!bmp.begin_I2C())
  {
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
  }
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);

  MDNSResponder mdns;
  MDNS.begin(HostName);
  MDNS.addService("http", "tcp", 80);

  website();



  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  NMEA2000.SetProductInformation("TPW01",
                                 107,
                                 "TPW Sensor Module",
                                 "2.1.0.0 (2024-07-06)",
                                 "2.0.0.0 (2023-01-26)"
                                );

  NMEA2000.SetDeviceInformation(8,
                                130,
                                25,
                                2046
                               );


  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);

  preferences.begin("nvs", false);
  NodeAddress = preferences.getInt("LastNodeAddress", 34);
  preferences.end();
  Serial.printf("NodeAddress=%d\n", NodeAddress);

  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, NodeAddress);
  NMEA2000.ExtendTransmitMessages(TransmitMessages);

  Serial.println(" opening NMEA2000");
  if (NMEA2000.Open())
    Serial.println(" NMEA2000 Initialized");
  else
    Serial.println(" NMEA2000 Initialized failed");

}

void loop()
{
  LEDoff();

  bConnect_CL = WiFi.status() == WL_CONNECTED ? 1 : 0;
  Serial.print("WL Connection Status: ");
  Serial.println(WL_CONNECTED);
  Serial.print("bConnect Client: ");
  Serial.println(bConnect_CL);
  {
    if (bConnect_CL == 1){
      Serial.printf("Wifi %s connencted!\n", CL_SSID);
      read_nmea0183();
      delay(100);
    }
    else{
      Serial.println("Wifi connect failed!\n");
      bConnect_CL = 0;
      Serial.printf("Reconnecting to %s\n", CL_SSID);
      WiFi.reconnect();
      delay(500);
      int WLcount = 0;
     int UpCount = 0;
      while (WiFi.status() != WL_CONNECTED && WLcount < 50){
        delay(50);
        Serial.printf(".");
        LEDflash(LED(Red));
        if (UpCount >= 20)
        {
          UpCount = 0;
          Serial.printf("\n");
        }
        ++UpCount;
        ++WLcount;
      }
      WiFiDiag();
    }
  }

 {
    switch (WiFi.status()){
      case 0: LEDblink(LED(Blue)); break;
      case 3: LEDblink(LED(Green)); break;
      case 4:
      case 5:
      case 6: LEDblink(LED(Red)); break;
      default: LEDoff();
    }
  }

ArduinoOTA.handle();


  Serial.printf("Soft-AP IP address: %s\n", WiFi.softAPIP().toString());
  sCL_Status = sWifiStatus(WiFi.status());

 delay(500);


     if (!bmp.performReading()) {
      Serial.println("Lesefehler BMP");
      sBMP_Status = "BMP Lesefehler";
      LEDflash(LED(Red));
      delay(200);
      return;
    }
    else {
      sBMP_Status = "BMP Lesen erfolgreich";
      fbmp_pressure = bmp.readPressure();
      fbmp_temperature = bmp.readTemperature();
      fbmp_altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
      LEDflash(LED(Blue));
      delay(100);
    }


    SendN2kWind();
    SendN2kPressure();
    SendN2kTemperatur();
    NMEA2000.ParseMessages();
    CheckSourceAddressChange();
}