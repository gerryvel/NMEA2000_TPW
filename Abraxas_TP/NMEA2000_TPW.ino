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
  on NMEA0183Handlers variable initialization. Use the MWD, MTW, DPT, MWV support
  NMEA0183Handler from AndrasSzep. So this does not automatically
  handle all NMEA0183 messages. If there is no handler for some message you need,
  you have to write handler for it and add it to the NMEA0183Handlers variable
  initialization.

// Version 1.1, 28.01.2022, gerryvel Gerry Sebb
*/

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Time.h>
#include <stdio.h>
#include <iostream>
#include "BoardInfo.h"
#include "configuration.h"
#include "helper.h"
#include <Adafruit_BMP280.h>
#include <esp.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LED.h"

// N2k
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>

// NMEA0183
#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include <NMEA0183Handlers.h>
#include <BoatData.h>

// Can Bus
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object

// BMP 280
Adafruit_BMP280 bmp280;

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Info Board for HTML-Output
String strBoardInfo;
BoardInfo boardInfo;

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
  // Serial.printf("\n handleData: data received from IP %s \n", client->remoteIP().toString().c_str());
  // Serial.printf("\n handleData: data received from Port %d \n", client->remotePort());
  Serial.write((uint8_t*)data, len);
  // Serial.printf("handleData: Len %i:\n", len);
  Serial.write((uint8_t*)data, len);

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
        NMEA0183_TCP.ParseTCPMessages(next_line, j);   //let's parse it
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

// formatted Output NMEA2000-Message
String NMEA_Info;
// To store last Node Address
int NodeAddress = 0;
// Nonvolatile storage on ESP32 - To store LastDeviceAddress
Preferences preferences;

// Set the information for other bus devices, which messages we support
const unsigned long TransmitMessages[] PROGMEM = { 130310L, // Outside Environmental parameters
                                                   130306L, // Wind
                                                   0
                                                 };

// Send time offsets
#define TempSendOffset 0
// Time between CAN Messages sent
#define SlowDataUpdatePeriod 1000

bool IsTimeToUpdate(unsigned long NextUpdate)
{
  return (NextUpdate < millis());
}
unsigned long InitNextUpdate(unsigned long Period, unsigned long Offset = 0)
{
  return millis() + Period + Offset;
}

void SetNextUpdate(unsigned long & NextUpdate, unsigned long Period)
{
  while (NextUpdate < millis()) NextUpdate += Period;
}

void SendN2kTempPressureWind(void)
{
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, TempSendOffset);
  tN2kMsg N2kMsg;

  if (IsTimeToUpdate(SlowDataUpdated))
  {
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

    bmp280_temperature = bmp280.readTemperature();
    bmp280_pressure = bmp280.readPressure();
    MWV_WindDirectionT = BoatData.WindDirectionT;  // Wind Relativ
    MWV_WindSpeedM = BoatData.WindSpeedM;
    VWR_WindDirectionM = BoatData.WindDirectionM;
    VWR_WindAngle = BoatData.WindAngle;
    VWR_WindSpeedkn = BoatData.WindSpeedK;

    Serial.printf("Temperatur: %3.1f °C - Luftdruck: %6.0f Pa\n", bmp280_temperature, bmp280_pressure);
    Serial.printf("WindT: %d ° - WindM: %d - SpeedM: %d - Angle: %d Pa\n", MWV_WindDirectionT, VWR_WindDirectionM, MWV_WindSpeedM, VWR_WindAngle);

    SetN2kPGN130310(N2kMsg, 0, N2kDoubleNA, CToKelvin(bmp280_temperature), bmp280_pressure);
    NMEA2000.SendMsg(N2kMsg);
    SetN2kPGN130306(N2kMsg, 0, MWV_WindSpeedM, MWV_WindDirectionT, tN2kWindReference::N2kWind_Apparent);
    NMEA2000.SendMsg(N2kMsg);
    SetN2kPGN130306(N2kMsg, 0, VWR_WindSpeedkn, VWR_WindDirectionM, tN2kWindReference::N2kWind_Magnetic);
    NMEA2000.SendMsg(N2kMsg);



    Serial.printf("%s\nData: %s\nPGN: %i\nPriority: %i\nSourceAdress: %i\n\n", "NMEA - Message:", (char*)N2kMsg.Data, (int)N2kMsg.PGN, (int)N2kMsg.Priority, (int)N2kMsg.Source);

    // Ausgabe für HTML-Seite zusammenstellen
    NMEA_Info = "<br>";
    NMEA_Info += "Data: ";
    NMEA_Info += (char*)N2kMsg.Data;
    NMEA_Info += "<br>";
    NMEA_Info += "DataLen: ";
    NMEA_Info += N2kMsg.DataLen;
    NMEA_Info += "<br>";
    NMEA_Info += "PNG: ";
    NMEA_Info += N2kMsg.PGN;
    NMEA_Info += "<br>";
    NMEA_Info += "Priority: ";
    NMEA_Info += N2kMsg.Priority;
    NMEA_Info += "<br>";
    NMEA_Info += "SourceAdress: ";
    NMEA_Info += N2kMsg.Source;
    NMEA_Info += "<br>";
    NMEA_Info += "Destination: ";
    NMEA_Info += N2kMsg.Destination;
  }
}

void WiFiDiag(void) {

  Serial.println("\nWifi-Diag:");
  AP_IP = WiFi.softAPIP();
  CL_IP = WiFi.localIP();
  Serial.print("AP IP address: ");
  Serial.println(AP_IP.toString());
  Serial.print("Client IP address: ");
  Serial.println(CL_IP.toString());
  WiFi.printDiag(Serial);
  Serial.print("\nScan AP's ");
  int n = WiFi.scanNetworks();
  Serial.printf("%i network(s) found\n", n);
  for (int i = 0; i < n; i++)
  {
    Serial.println(WiFi.SSID(i));
  }
  Serial.println();
}

// WiFiMulti wifiMulti;
AsyncClient* client;

//===============================================SETUP==============================
void setup()
{
  Serial.begin(115200);

  strBoardInfo = boardInfo.ShowChipIDtoString();

  // LED init
  LEDInit();
  LEDoff();

  //WiFiServer AP starten
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(1000);
  if (WiFi.softAPConfig(IP, Gateway, NMask))
    Serial.println("IP config success");
  else
    Serial.println("IP config not success");

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP configured with address: ");
  Serial.println(myIP);

  // Webserver start
  server.begin();

  int count = 0;

  // Anmelden mit WiFi als Client an AP
  //	WiFi.mode(WIFI_STA);
  WiFi.begin(CL_SSID, CL_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    count++;
    if (count = 10) break;
  }
  if (WiFi.isConnected()) {
    Connect_CL = 1;
    Serial.println("Client Connection");
  }
  else
    Serial.println("Client Connection failed");
    digitalWrite(LED(Red), HIGH);

  // Autoconnect
  WiFi.setAutoConnect(true);
  WiFi.persistent(true);
  delay(500);

  client = new AsyncClient;
  client->onData(&handleData, client);
  client->onConnect(&onConnect, client);
  client->connect(SERVER_HOST_NAME, TCP_PORT);

  ets_timer_disarm(&intervalTimer);
  ets_timer_setfn(&intervalTimer, &replyToServer, client);

  bmp280.begin();

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";
    Serial.println("start updating " + type);
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

  ArduinoOTA.setHostname("NMEA2000_TPW");
  ArduinoOTA.begin();

  // Setup NMEA2000 system
  // Reserve enough buffer for sending all messages. This does not work on small memory devices like Uno or Mega
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  NMEA2000.SetProductInformation("1", // Manufacturer's Model serial code
                                 107, // Manufacturer's product code
                                 "NMEA2000_TPW",  // Manufacturer's Model ID
                                 "1.0.1.0 (2021-01-26)",  // Manufacturer's Software version code
                                 "1.0.0.0 (2021-01-26)" // Manufacturer's Model version
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
{ // Reconnect Wifi
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Connect_CL == 0) {
      Connect_CL = 1;
      Serial.println("Wifi reconnencted!\n");
    }
    // and listen from NMEA0183 Stream
    client->onData(&handleData, client);
    client->onConnect(&onConnect, client);
    client->connect(SERVER_HOST_NAME, TCP_PORT);
    client->close();
    digitalWrite(LED(Green), HIGH);
    delay(500);
  }
  else
  {
    Serial.println("Wifi reconnect failed!\n");
    Connect_CL = 0;
    WiFi.begin(CL_SSID, CL_PASSWORD);    // wifi down, reconnect here
    delay(500);
    int WLcount = 0;
    int UpCount = 0;
    while (WiFi.status() != WL_CONNECTED && WLcount < 100)
    {
      delay(100);
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

  if (Connect_CL == 1) {
  		WiFiDiag();
  }

  ArduinoOTA.handle();

  {
    SendN2kTempPressureWind();

    NMEA2000.ParseMessages();
    int SourceAddress = NMEA2000.GetN2kSource();
    if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
      preferences.begin("nvs", false);
      preferences.putInt("LastNodeAddress", SourceAddress);
      preferences.end();
      Serial.printf("Address Change: New Address=%d\n", SourceAddress);
    }
  }

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client)
  { // If a new client connects,
    Serial.println("New WebClient connected.");   // print a message out in the serial port
    String currentLine = "";                     // make a String to hold incoming data from the client
    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {
        // IP Adresse Client
        SELF_IP = client.remoteIP();
        // BMP Altitude
        bmp280_altitude = bmp280.readAltitude();

        // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println(SendHTML(AP_IP, SELF_IP, CL_IP, CL_SSID, bmp280_temperature, bmp280_pressure, bmp280_altitude, NMEA_Info, MWV_WindDirectionT, MWV_WindSpeedM, strBoardInfo));
            client.println();  // The HTTP response ends with another blank line
            break; // Break out of the while loop
          }
          else {							// if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r') {				 // if you got anything else but a carriage return character,
          currentLine += c;				 // add it to the end of the currentLine
        }
      }
    }
    header = "";								// Clear the header variable
    client.stop();								// Close the connection
    Serial.println("WebClient disconnected.");
    Serial.println("");
  }
}
