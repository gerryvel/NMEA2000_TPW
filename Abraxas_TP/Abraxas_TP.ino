#include <Arduino.h>
#include <ArduinoOTA.h>
#include "helper.h"
#include "BoardInfo.h"
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_BMP280.h>
#include <esp.h>
#include <Preferences.h>
#include "BoatData.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// N2k
#include <NMEA2000_CAN.h>
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>

// NMEA0183
#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include "NMEA0183Handlers.h"

// BMP 280
Adafruit_BMP280 bmp280;

// Set IP for WIFI-AP
IPAddress IP = IPAddress(192, 168, 15, 1);
IPAddress Gateway = IPAddress(192, 168, 15, 1);
IPAddress NMask = IPAddress(255, 255, 255, 0);
IPAddress AP_IP;
IPAddress CL_IP;
IPAddress SELF_IP;

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
char next_line[MAX_NMEA0183_MSG_BUF_LEN]; //NMEA0183 message buffer
size_t i = 0, j = 1;                          //indexers
uint8_t *pointer_to_int;                  //pointer to void *data (!)

static void replyToServer(void* arg) {
	AsyncClient* client = reinterpret_cast<AsyncClient*>(arg);

	// send reply
	if (client->space() > 32 && client->canSend()) {
		char message[32];
		sprintf(message, "this is from %s", WiFi.localIP().toString().c_str());
		Serial.println(message);
		client->add(message, strlen(message));
		if (client->send())
			Serial.println("Client send ok");
		else Serial.println("Client send not ok");
	}
}

/************************ event callbacks ***************************/
static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
			  Serial.printf("\n data received from %s \n", client->remoteIP().toString().c_str());
			  Serial.write((uint8_t*)data, len);
			  Serial.printf("%i: ", len);  
			  Serial.write((uint8_t*)data, len);

			uint8_t *pointer_to_int;
			pointer_to_int = (uint8_t *)data;

			if (len == 1) {                  //in case just one byte was received for whatever reason
				next_line[0] = pointer_to_int[0];
				j = 1;
				    Serial.printf("%c;", next_line[0]);
			}
			else {
				for (i = 0; i < len; i++) {
					next_line[j++] = pointer_to_int[i];
					if (pointer_to_int[i - 1] == 13 && pointer_to_int[i] == 10) {
						next_line[j] = 0;
						        Serial.printf("%i: %s", j, next_line);    //here we got the full line ending CRLF
						NMEA2000.ParseMessages();
						NMEA0183_TCP.ParseTCPMessages(next_line, j);   //let's parse it     
						j = 0;
					}
				}
			}


			ets_timer_arm(&intervalTimer, 2000, true); // schedule for reply to server at next 2s
		}

		void onConnect(void* arg, AsyncClient* client) {
			Serial.printf("\n NMEA0183 listener has been connected to %s on port %d \n", SERVER_HOST_NAME, TCP_PORT);
			replyToServer(client);
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


void SendN2kTempPressure(void)
{
	static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, TempSendOffset);
	tN2kMsg N2kMsg;

	if (IsTimeToUpdate(SlowDataUpdated))
	{
		SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

		bmp280_temperature = bmp280.readTemperature();
		bmp280_pressure = bmp280.readPressure();
		NMEA0183_WindDirectionT = BoatData.WindDirectionT;
		NMEA0183_WindDirectionM = BoatData.WindDirectionM;
		NMEA0183_WindSpeedM = BoatData.WindSpeedM;
		NMEA0183_WindAngle = BoatData.WindAngle;
	
		Serial.printf("Temperatur: %3.1f °C - Luftdruck: %6.0f Pa\n", bmp280_temperature, bmp280_pressure);
		Serial.printf("WindT: %d ° - WindM: %d - SpeedM: %d - Angle: %d Pa\n", NMEA0183_WindDirectionT, NMEA0183_WindDirectionM, NMEA0183_WindSpeedM, NMEA0183_WindAngle);


		SetN2kPGN130310(N2kMsg, 0, N2kDoubleNA, CToKelvin(bmp280_temperature), bmp280_pressure);
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

void WiFiDiag(void){
	
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

/* clients events */
static void handleError(void* arg, AsyncClient* client, int8_t error) {
	Serial.printf("\n connection error %s from client %s \n", client->errorToString(error), client->remoteIP().toString().c_str());
}

void setup()
{
	Serial.begin(115200);
	//ShowTime();

	printf("Anzeige aktualisieren alle %i Sekunden.", PAGE_REFRESH);

	// ID des Boards anzeigen
	boardInfo.ShowChipID();
	boardInfo.ShowChipInfo();
	boardInfo.ShowChipTemperature();
	strBoardInfo = boardInfo.ShowChipIDtoString();

	//WiFiServer AP starten
	WiFi.softAP(AP_SSID, AP_PASSWORD);
	delay(1000);

	if (WiFi.softAPConfig(IP, Gateway, NMask))
		Serial.println("AP config success");
	else Serial.println("AP config not success");

	if (WiFi.softAPsetHostname(HostName))
		Serial.println("Hostname set success");
	else Serial.println("Hostname set not success");

	//Connect to other AP
	WiFi.mode(WIFI_AP_STA);
	Serial.printf("Connecting to %s:", CL_SSID);
	WiFi.begin(CL_SSID, CL_PASSWORD);
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print('.');
		delay(500);
	}
	Serial.println(" connected!");

	WiFiDiag();

	AsyncClient* client = new AsyncClient;
	client->onData(&handleData, client);
	client->onError(&handleError, NULL);
	client->onConnect(&onConnect, client);
	client->connect(SERVER_HOST_NAME, TCP_PORT);

	ets_timer_disarm(&intervalTimer);
	ets_timer_setfn(&intervalTimer, &replyToServer, client);
	
	server.begin();
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

		ArduinoOTA.begin();

		
	// Setup NMEA0183 ports and handlers
	InitNMEA0183Handlers(&NMEA2000, &BoatData);
	NMEA0183_TCP.SetMsgHandler(HandleNMEA0183Msg);
	NMEA0183_TCP.SetMessageStream(&Serial);   //just to have IsOpen() valid
	NMEA0183_TCP.Open();
	Serial.println(" NMEA0183 Initialized\n");
	


	// Setup NMEA2000 system
	// Reserve enough buffer for sending all messages. This does not work on small memory devices like Uno or Mega
	NMEA2000.SetN2kCANMsgBufSize(8);
	NMEA2000.SetN2kCANReceiveFrameBufSize(250);
	NMEA2000.SetN2kCANSendFrameBufSize(250);

	// Set product information
	NMEA2000.SetProductInformation("1", // Manufacturer's Model serial code
		100, // Manufacturer's product code
		"Abraxas TP",  // Manufacturer's Model ID
		"1.0.3.01 (2020-10-04)",  // Manufacturer's Software version code
		"1.0.2.0 (2020-10-03)" // Manufacturer's Model version
	);
	// Set device information
	NMEA2000.SetDeviceInformation(ESP.getEfuseMac(), // Unique number. Use e.g. Serial number.
		130, // Device function=Temperature. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
		75, // Device class=Sensor Communication Interface. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
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

	NMEA2000.Open();

	delay(200);

}

void loop() {
	
	ArduinoOTA.handle();

	SendN2kTempPressure();

	NMEA2000.ParseMessages();
	int SourceAddress = NMEA2000.GetN2kSource();
	if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
		preferences.begin("nvs", false);
		preferences.putInt("LastNodeAddress", SourceAddress);
		preferences.end();
		Serial.printf("Address Change: New Address=%d\n", SourceAddress);
	}

	WiFiClient client = server.available();   // Listen for incoming clients

	if (client)
	{                                                // If a new client connects,
		Serial.println("Neuer Client connected.");   // print a message out in the serial port
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

						client.println(SendHTML(AP_IP, SELF_IP, CL_IP, CL_SSID, bmp280_temperature, bmp280_pressure, bmp280_altitude, NMEA_Info, NMEA0183_WindDirectionT, NMEA0183_WindSpeedM, strBoardInfo));

						// The HTTP response ends with another blank line
						client.println();
						// Break out of the while loop
						break;
					}
					else { // if you got a newline, then clear currentLine
						currentLine = "";
					}
				}
				else if (c != '\r') {  // if you got anything else but a carriage return character,
					currentLine += c;      // add it to the end of the currentLine
				}
			}
		}
		// Clear the header variable
		header = "";
		// Close the connection
		client.stop();
		Serial.println("Client disconnected.");
		Serial.println("");
	}
}
