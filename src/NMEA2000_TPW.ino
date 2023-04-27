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

  NMEA2000 Temperature and Barometric Pressure with BMP280.
  Reads messages from NMEA0183 WindSensor and forwards them to the N2k bus.
  The messages, which will be handled has been defined on NMEA0183Handlers.cpp
  on NMEA0183Handlers variable initialization. So this does not automatically
  handle all NMEA0183 messages. If there is no handler for some message you need,
  you have to write handler for it and add it to the NMEA0183Handlers variable
  initialization. NMEA0183.h add ParseTCPMessage function from Andras Szap.

  // Version 1.3, 24.03.2023, gerryvel Gerry Sebb
*/

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

// N2k
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>

// NMEA0183
#include "NMEA0183.h"
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include "NMEA0183handlers.h"
#include <BoatData.h>

// Can Bus
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object

// BMP 300
Adafruit_BMP3XX bmp;

// Set web server port number to 80
AsyncWebServer server(80);
AsyncEventSource events("/events");

// Info Board for HTML-Output
BoardInfo boardInfo;
String sBoardInfo;

//Variables for websit
String sCL_Status = sWifiStatus(WiFi.status());
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
char next_line[81]; //NMEA0183 message buffer
size_t i = 0, j = 1;                          //indexers
uint8_t *pointer_to_int;                  //pointer to void *data (!)

static void replyToServer(void* arg)
{
  AsyncClient* client = reinterpret_cast<AsyncClient*>(arg);

  // send reply
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

/************************ event callbacks ***************************/
static void handleData(void* arg, AsyncClient* client, void *data, size_t len)
{
  Serial.printf("\n handleData: data received from IP %s \n", client->remoteIP().toString().c_str());
  Serial.printf("\n handleData: data received from Port %d \n", client->remotePort());
  Serial.write((uint8_t*)data, len);
  // Serial.printf("handleData: Len %i:\n", len);

  uint8_t *pointer_to_int;
  pointer_to_int = (uint8_t *)data;

  if (len == 1)
  { //in case just one byte was received for whatever reason
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
        Serial.printf("%s", next_line);    //here we got the full line ending CRLF
        NMEA2000.ParseMessages();
        NMEA0183_TCP.ParseTCPMessages(next_line, j);   //let's parse it (next_line, j)
        j = 0;
      }
    }
  }
  ets_timer_arm(&intervalTimer, 2000, true); // schedule for reply to server at next 2s
}

void onConnect(void* arg, AsyncClient* client)
{
  Serial.printf("\nNMEA0183 listener Status %i \n\n", client->state());
}


String NMEA_Info; // formatted Output NMEA2000-Message
int NodeAddress = 0; // To store last Node Address
Preferences preferences; // Nonvolatile storage on ESP32 - To store LastDeviceAddress
// Set the information for other bus devices, which messages we support
const unsigned long TransmitMessages[] PROGMEM = { 130310L, // Outside Environmental parameters
                                                   130306L, // Wind
                                                   0
                                                 };

#define TempSendOffset 0 // Send time offsets
#define SlowDataUpdatePeriod 1000 // Time between CAN Messages sent

bool IsTimeToUpdate(unsigned long NextUpdate){
  return (NextUpdate < millis());
}
unsigned long InitNextUpdate(unsigned long Period, unsigned long Offset = 0){
  return millis() + Period + Offset;
}

void SetNextUpdate(unsigned long & NextUpdate, unsigned long Period){
  while (NextUpdate < millis()) NextUpdate += Period;
}

void SendN2kTempPressureWind(void){
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, TempSendOffset);
  tN2kMsg N2kMsg;

  if (IsTimeToUpdate(SlowDataUpdated)){
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

    dMWV_WindDirectionT = BoatData.WindDirectionT;  // Wind Relativ
    dMWV_WindSpeedM = BoatData.WindSpeedM;
    dVWR_WindDirectionM = BoatData.WindDirectionM;
    dVWR_WindAngle = BoatData.WindAngle;
    dVWR_WindSpeedkn = BoatData.WindSpeedK;

    Serial.printf("Temperatur: %3.1f °C - Luftdruck: %3.2f hPa - Hoehe: %3.0f m\n", fbmp_temperature, fbmp_pressure/100, fbmp_altitude);
    Serial.printf("WindT: %f ° - WindM: %f - SpeedM: %f - Angle: %f °\n", dMWV_WindDirectionT, dVWR_WindDirectionM, dMWV_WindSpeedM, dVWR_WindAngle);

    SetN2kPGN130310(N2kMsg, 0, N2kDoubleNA, CToKelvin(fbmp_temperature), fbmp_pressure);
    NMEA2000.SendMsg(N2kMsg);
    SetN2kPGN130306(N2kMsg, 0, dMWV_WindSpeedM, dMWV_WindDirectionT, tN2kWindReference::N2kWind_Apparent);
    NMEA2000.SendMsg(N2kMsg);
    SetN2kPGN130306(N2kMsg, 0, dVWR_WindSpeedkn, dVWR_WindDirectionM, tN2kWindReference::N2kWind_Magnetic);
    NMEA2000.SendMsg(N2kMsg);

    Serial.printf("%s\nData: %s\nPGN: %i\nPriority: %i\nSourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);
  }
}

AsyncClient* client;

//===============================================SETUP==============================
void setup()
{
  Serial.begin(115200);

//Filesystem prepare for Webfiles
	if (!LittleFS.begin(true)) {
		Serial.println("An Error has occurred while mounting LittleFS");
		return;
	}
	Serial.println("Speicher LittleFS benutzt:");
	Serial.println(LittleFS.usedBytes());

	File root = LittleFS.open("/");
  listDir(LittleFS, "/", 3);

  freeHeapSpace();

// I2C
  Wire.begin(BMP_SDA, BMP_SCL);
  I2C_scan();
//LED
  LEDInit();
  LEDoff();
// Boardinfo	
  sBoardInfo = boardInfo.ShowChipIDtoString();

//Wifi
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
  
/*
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.htm");

  server.onNotFound([](AsyncWebServerRequest *request)
  {
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }
   int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });
*/
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

// Anmelden mit WiFi als Client an Windmesser
  WiFi.disconnect(true);
    delay(1000);   
    int count = 0; // Init Counter WFIConnect  
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
    
// Start TCP (HTTP) server
	server.begin();
	Serial.println("TCP server started");

  client = new AsyncClient;
  client->onData(&handleData, client);
  client->onConnect(&onConnect, client);
  client->connect(SERVER_HOST_NAME, TCP_PORT);

  ets_timer_disarm(&intervalTimer);
  ets_timer_setfn(&intervalTimer, &replyToServer, client);

// BMP388 begin & setup
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


  MDNS.begin(HostName);
  MDNS.addService("http", "tcp", 80);

  // Setup NMEA2000 system
  // Reserve enough buffer for sending all messages. This does not work on small memory devices like Uno or Mega
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  NMEA2000.SetProductInformation("1", // Manufacturer's Model serial code
                                 107, // Manufacturer's product code
                                 "NMEA2000TPW",  // Manufacturer's Model ID
                                 "2.0.0.0 (2023-01-26)",  // Manufacturer's Software version code
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

  // Setup NMEA0183 ports and handlers
  InitNMEA0183Handlers(&NMEA2000, &BoatData);

  NMEA0183_TCP.SetMsgHandler(HandleNMEA0183Msg);
  NMEA0183_TCP.SetMessageStream(&Serial);   //just to have IsOpen() valid

  if (NMEA0183_TCP.Open())
    Serial.println(" NMEA0183 Initialized");
  else
    Serial.println(" NMEA0183 Initialized failed");
  client->close();
  
}
//===============================================Loop==============================
void loop()
{ 
  LEDoff();
  // Listen NMEA0183 or reconnect Wifi
  bConnect_CL = WiFi.status() == WL_CONNECTED ? 1 : 0;
  Serial.printf("WL Connection Status: %i\n", WL_CONNECTED);
  Serial.printf("bConnect Client: %i\n", bConnect_CL);
  //iSTA_on = WiFi.enableSTA(iSTA_on);
  { // Listen NMEA0183
    if (bConnect_CL == 1){ // Connected an listen
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
      WiFi.reconnect();    // wifi down, reconnect here
      delay(500);
      int WLcount = 0;
      int UpCount = 0;
      while (WiFi.status() != WL_CONNECTED && WLcount < 100){
        delay(500);
        Serial.printf(".");
        LEDflash(LED(Red));
        if (UpCount >= 60)  // just keep terminal from scrolling sideways
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

 { // LED visu Wifi
    switch (WiFi.status()){
      case 0: LEDblink(LED(Blue)); break;  // WL_IDLE_STATUS
      case 3: LEDblink(LED(Green)); break;  // WL_CONNECTED
      case 4:                              // WL_CONNECT_FAILED
      case 5:                              // WL_CONNECTION_LOST
      case 6: LEDblink(LED(Red)); break;  // WL_DISCONNECTED
      default: LEDoff();
    }
  }

  ArduinoOTA.handle();

// Status AP 
  Serial.printf("Stationen mit AP verbunden = %d\n", WiFi.softAPgetStationNum());
  Serial.printf("Soft-AP IP address = %s\n", WiFi.softAPIP().toString());

  sCL_Status = sWifiStatus(WiFi.status());


 //Read BMP
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

  //N2K
    SendN2kTempPressureWind();

    NMEA2000.ParseMessages();
    int SourceAddress = NMEA2000.GetN2kSource();
    if (SourceAddress != NodeAddress)
    { // Save potentially changed Source Address to NVS memory
      preferences.begin("nvs", false);  // NVS=EEPROM
      preferences.putInt("LastNodeAddress", SourceAddress);
      preferences.end();
      Serial.printf("Address Change: New Address=%d\n", SourceAddress);
    }
  
}
