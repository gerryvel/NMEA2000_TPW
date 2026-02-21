# 1 "C:\\Users\\GERRYA~1\\AppData\\Local\\Temp\\tmp1fif357j"
#include <Arduino.h>
# 1 "C:/Users/gerryadmin/Documents/NMEA2000_TPW/src/NMEA2000_TPW.ino"
# 20 "C:/Users/gerryadmin/Documents/NMEA2000_TPW/src/NMEA2000_TPW.ino"
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


#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>


#include "NMEA0183.h"
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include <NMEA0183Stream.h>


#include <NMEA2000_CAN.h>


Adafruit_BMP280 bmp280;
Adafruit_BMP3XX bmp3xx;

static ETSTimer intervalTimer;



String NMEA_Info;
int NodeAddress = 0;
Preferences preferences;


const unsigned long TransmitMessages[] PROGMEM = { 130306L,
                                                   130314L,
                                             130316L,
                                                   0
                                                 };
bool IsTimeToUpdate(unsigned long NextUpdate);
void SetNextUpdate(unsigned long & NextUpdate, unsigned long Period);
void CheckSourceAddressChange();
void SendN2kWind(void);
void SendN2kCabinTemperatur(void);
void SendN2kOutdoorTemperatur(void);
void SendN2kPressure(void);
void setup();
void loop();
#line 73 "C:/Users/gerryadmin/Documents/NMEA2000_TPW/src/NMEA2000_TPW.ino"
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
    Serial.printf("Windrichtung: %f ° - Windgeschw: %f m/s, Referenz: %i\n", dMWV_WindAngle, dMWV_WindSpeed, dMWV_Reference);

    SetN2kPGN130306(N2kMsg, 0, dMWV_WindSpeed, dMWV_WindAngle, tN2kWindReference(dMWV_Reference));
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
    Serial.printf("Temperatur: %3.1f °C\n", fbmp_temperature);

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
    Serial.printf("Temperatur: %3.1f °C\n", fWindSensorTemp);

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


void setup()
{
  Serial.begin(115200);

  Serial.printf("TPW Sensor setup %s start\n", Version);


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

  Serial.println("Configdata WIFI-Client:\n Client SSID: " + CL_SSID + " , Passwort: " + CL_PASSWORD + " , Sensortyp: " + Sensortyp);


  Wire.begin(BMP_SDA, BMP_SCL);
  I2C_scan();

  LEDInit();

  sBoardInfo = boardInfo.ShowChipIDtoString();


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


Serial.println("Client Connection");
  WiFi.disconnect(true);
  delay(1000);
  int count = 0;

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
    Serial.println("Connection to NMEA-Server ready");
  } else {
    Serial.println("Connection to NMEA-Server failure");
  }


  NMEA0183.SetMessageStream(&nmeaclient);
  NMEA0183.Open();
  Serial.println("NMEA0183 started");


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




  MDNSResponder mdns;
  MDNS.begin(HostName);
  MDNS.addService("http", "tcp", 80);

  website();



  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  NMEA2000.SetProductInformation("TPW 01",
                                 107,
                                 "TPW Sensor Module",
                                 "2.2.0.0 (2024-08-22)",
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


  LEDoff_RGB();
}

void loop()
{

  bConnect_CL = WiFi.status() == WL_CONNECTED ? 1 : 0;
  Serial.printf("WLAN Status: %s \n", sWifiStatus(WiFi.status()));
  Serial.printf("bConnect Client: %i \n", bConnect_CL);
  {
    if (bConnect_CL == 1){
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
      WiFi.reconnect();
      delay(500);
      int WLcount = 0;
     int UpCount = 0;
      while (WiFi.status() != WL_CONNECTED && WLcount < 50){
        delay(50);
        Serial.printf(".");

        flashLED(LED(Red), 5);
        if (UpCount >= 20)
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


  Serial.printf("Soft-AP IP address: %s\n", WiFi.softAPIP().toString());
  sCL_Status = sWifiStatus(WiFi.status());

 delay(500);


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

      flashLED(LED(Blue), 5);
      delay(100);
    }
  }


    SendN2kWind();
    SendN2kPressure();
    SendN2kCabinTemperatur();
    SendN2kOutdoorTemperatur();
    NMEA2000.ParseMessages();
    CheckSourceAddressChange();

    freeHeapSpace();

if (IsRebootRequired) {
  Serial.println("Rebooting ESP32: ");
  delay(1000);
  ESP.restart();
  }
}