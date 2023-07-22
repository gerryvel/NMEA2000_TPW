# 1 "C:\\Users\\GERRYA~1\\AppData\\Local\\Temp\\tmpun1cdun6"
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


#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>


#include "NMEA0183.h"
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include "NMEA0183handlers.h"
#include <BoatData.h>


#include <NMEA2000_CAN.h>


Adafruit_BMP3XX bmp;


AsyncWebServer server(80);
AsyncEventSource events("/events");


BoardInfo boardInfo;
String sBoardInfo;


String sCL_Status = sWifiStatus(WiFi.status());
String replaceVariable(const String& var);
static void replyToServer(void* arg);
static void handleData(void* arg, AsyncClient* client, void *data, size_t len);
void onConnect(void* arg, AsyncClient* client);
bool IsTimeToUpdate(unsigned long NextUpdate);
void SetNextUpdate(unsigned long & NextUpdate, unsigned long Period);
void CheckSourceAddressChange();
void SendN2kWind(void);
void SendN2kTemperatur(void);
void SendN2kPressure(void);
void setup();
void loop();
#line 69 "C:/Users/gerryadmin/Documents/PlatformIO/Projects/NMEA2000_TPW/src/NMEA2000_TPW.ino"
String replaceVariable(const String& var){
 if (var == "sWDirection")return String(dMWV_WindDirectionT,1);
 if (var == "sWGaugeDirection")return String(dMWV_WindDirectionT, 1);
 if (var == "sWSpeed")return String(dMWV_WindSpeedM,1);
 if (var == "sTemp")return String(fbmp_temperature, 1);
  if (var == "sPress")return String(fbmp_pressure/100, 0);
 if (var == "sBoardInfo")return sBoardInfo;
  if (var == "sFS_Space")return String(LittleFS.usedBytes());
 if (var == "sAP_IP")return AP_IP.toString();
  if (var == "sAP_Clients")return String(WiFi.softAPgetStationNum());
  if (var == "sCL_Addr")return WiFi.localIP().toString();
  if (var == "sCL_Status")return String(sCL_Status);
  if (var == "sBMP_Status")return String(sBMP_Status);
  return "NoVariable";
}

tBoatData BoatData;
tNMEA0183 NMEA0183_TCP;

static ETSTimer intervalTimer;
char next_line[81];
size_t i = 0, j = 1;
uint8_t *pointer_to_int;

static void replyToServer(void* arg)
{
  AsyncClient* client = reinterpret_cast<AsyncClient*>(arg);


  if (client->space() > 32 && client->canSend())
  {
    char message[50];
    sprintf(message, "this is from %s", WiFi.localIP().toString().c_str());
    client->add(message, strlen(message));
    client->send();
    Serial.println(message);
    Serial.printf("\nReplyToServer: data received from %s \n", client->remoteIP().toString().c_str());
  }
}


static void handleData(void* arg, AsyncClient* client, void *data, size_t len)
{
  Serial.printf("\n handleData: data received from IP %s \n", client->remoteIP().toString().c_str());
  Serial.printf("\n handleData: data received from Port %d \n", client->remotePort());
  Serial.write((uint8_t*)data, len);


  uint8_t *pointer_to_int;
  pointer_to_int = (uint8_t *)data;

  if (len == 1)
  {
    next_line[0] = pointer_to_int[0];
    j = 1;
    Serial.printf("%c;", next_line[0]);
  }
  else
  {
    for (i = 0; i < len; i++)
    {
      next_line[j++] = pointer_to_int[i];
      if (pointer_to_int[i - 1] == 13 && pointer_to_int[i] == 10)
      {
        next_line[j] = 0;
        Serial.printf("%s", next_line);
        NMEA2000.ParseMessages();
        NMEA0183_TCP.ParseTCPMessages(next_line, j);
        j = 0;
      }
    }
  }
  ets_timer_arm(&intervalTimer, 2000, true);
}

void onConnect(void* arg, AsyncClient* client)
{
  Serial.printf("\nNMEA0183 listener Status %i \n\n", client->state());
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

#define TempSendOffset 0
#define PressSendOffset 50
#define WindSendOffset 100
#define SlowDataUpdatePeriod 1000

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

    dMWV_WindDirectionT = BoatData.WindDirectionT;
    dMWV_WindSpeedM = BoatData.WindSpeedM;
    dVWR_WindDirectionM = BoatData.WindDirectionM;
    dVWR_WindAngle = BoatData.WindAngle;
    dVWR_WindSpeedms = BoatData.WindSpeedM;

    Serial.printf("WindT: %f ° - WindM: %f - SpeedM: %f - Angle: %f °\n", dMWV_WindDirectionT, dVWR_WindDirectionM, dMWV_WindSpeedM, dVWR_WindAngle);

    SetN2kPGN130306(N2kMsg, 0, dVWR_WindSpeedms, dVWR_WindAngle ,tN2kWindReference::N2kWind_Apparent);
    NMEA2000.SendMsg(N2kMsg);



    Serial.printf("%s\nData: %s\nPGN: %i\nPriority: %i\nSourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
  }
}


void SendN2kTemperatur(void){
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, TempSendOffset);
  tN2kMsg N2kMsg;

  if (IsTimeToUpdate(SlowDataUpdated)){
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

    Serial.printf("Temperatur: %3.1f °C - Luftdruck: %3.2f hPa - Hoehe: %3.0f m\n", fbmp_temperature, fbmp_pressure/100, fbmp_altitude);

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

    Serial.printf("Temperatur: %3.1f °C - Luftdruck: %3.2f hPa - Hoehe: %3.0f m\n", fbmp_temperature, fbmp_pressure/100, fbmp_altitude);

    SetN2kPGN130314(N2kMsg, 0, 0, N2kps_Atmospheric, (fbmp_pressure));
    NMEA2000.SendMsg(N2kMsg);

    Serial.printf("%s\nData: %s\nPGN: %i\nPriority: %i\nSourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
  }
}

AsyncClient* client;


void setup()
{
  Serial.begin(115200);


 if (!LittleFS.begin(true)) {
  Serial.println("An Error has occurred while mounting LittleFS");
  return;
 }
 Serial.println("Speicher LittleFS benutzt:");
 Serial.println(LittleFS.usedBytes());

 File root = LittleFS.open("/");
  listDir(LittleFS, "/", 3);

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
    IPAddress myIP = WiFi.softAPIP();
    Serial.println("Network " + String(AP_SSID) + " running");
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    delay(1000);
  } else {
      Serial.println("Starting AP failed.");
      digitalWrite(LED(Red), HIGH);
      delay(1000);
      ESP.restart();
  }

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon.ico", "image/x-icon");
 });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
  request->send(LittleFS, "/index.html", String(), false, replaceVariable);
 });
 server.on("/system.html", HTTP_GET, [](AsyncWebServerRequest* request) {
  request->send(LittleFS, "/system.html", String(), false, replaceVariable);
 });
 server.on("/ueber.html", HTTP_GET, [](AsyncWebServerRequest* request) {
  request->send(LittleFS, "/ueber.html", String(), false, replaceVariable);
 });
 server.on("/gauge.min.js", HTTP_GET, [](AsyncWebServerRequest* request) {
  request->send(LittleFS, "/gauge.min.js");
 });
 server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->send(LittleFS, "/style.css", "text/css");
 });


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
    Serial.println("Client Connection");
    LEDflash(LED(Green));
  }
  else
    Serial.println("Client Connection failed");
    WiFi.reconnect();


 server.begin();
 Serial.println("TCP server started");

  client = new AsyncClient;
  client->onData(&handleData, client);
  client->onConnect(&onConnect, client);
  client->connect(SERVER_HOST_NAME, TCP_PORT);

  ets_timer_disarm(&intervalTimer);
  ets_timer_setfn(&intervalTimer, &replyToServer, client);


  if (!bmp.begin_I2C())
  {
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
  }
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);


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


  MDNS.begin(HostName);
  MDNS.addService("http", "tcp", 80);



  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  NMEA2000.SetProductInformation("1",
                                 107,
                                 "NMEA2000TPW",
                                 "2.0.0.0 (2023-01-26)",
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


  InitNMEA0183Handlers(&NMEA2000, &BoatData);

  NMEA0183_TCP.SetMsgHandler(HandleNMEA0183Msg);
  NMEA0183_TCP.SetMessageStream(&Serial);

  if (NMEA0183_TCP.Open())
    Serial.println(" NMEA0183 Initialized");
  else
    Serial.println(" NMEA0183 Initialized failed");
  client->close();

}

void loop()
{
  LEDoff();

  bConnect_CL = WiFi.status() == WL_CONNECTED ? 1 : 0;
  Serial.printf("WL Connection Status: %i\n", WL_CONNECTED);
  Serial.printf("bConnect Client: %i\n", bConnect_CL);

  {
    if (bConnect_CL == 1){
      Serial.printf("Wifi %s connencted!\n", CL_SSID);
      client->onData(&handleData, client);
      client->onConnect(&onConnect, client);
      client->connect(SERVER_HOST_NAME, TCP_PORT);
      client->close();
      delay(500);
    }
    else{
      Serial.println("Wifi connect failed!\n");
      bConnect_CL = 0;
      Serial.printf("Reconnecting to %s\n", CL_SSID);
      WiFi.disconnect();
      WiFi.reconnect();
      delay(500);
      int WLcount = 0;
      int UpCount = 0;
      while (WiFi.status() != WL_CONNECTED && WLcount < 50){
        delay(500);
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


  Serial.printf("Stationen mit AP verbunden = %d\n", WiFi.softAPgetStationNum());
  Serial.printf("Soft-AP IP address = %s\n", WiFi.softAPIP().toString());

  sCL_Status = sWifiStatus(WiFi.status());



     if (!bmp.performReading()) {
      Serial.println("Lesefehler BMP");
      sBMP_Status = "BMP Lesefehler";
      LEDflash(LED(Red));
      delay(100);
      return;
    }
    else
      sBMP_Status = "BMP Lesen erfolgreich";

    fbmp_pressure = bmp.readPressure();
    fbmp_temperature = bmp.readTemperature();
    fbmp_altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
    LEDflash(LED(Blue));
    delay(500);


    SendN2kWind();
    SendN2kPressure();
    SendN2kTemperatur();
    NMEA2000.ParseMessages();
    CheckSourceAddressChange();

}