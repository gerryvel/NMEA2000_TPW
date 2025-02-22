#ifndef _HELPER_H_
#define _HELPER_H_


#include <stdio.h>
#include <time.h>
#include <Arduino.h>
#include <LITTLEFS.h>
#include <FS.h>
#include <Wire.h>
#include <WiFi.h>
#include "configuration.h"
#include <ArduinoJson.h>
#include <Preferences.h>

void ShowTime(){
	time_t now = time(NULL);
	struct tm tm_now;
	localtime_r(&now, &tm_now);
	char buff[100];
	strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S", &tm_now);
	printf("Zeit: %s\n", buff);
}

void freeHeapSpace(){
	static unsigned long last = millis();
	if (millis() - last > 5000) {
		last = millis();
    sHeapspace = ESP.getFreeHeap();
		Serial.printf("\n[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
	}
}
/***************************** WiFi Diagnose **************************/
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
  {
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) 
        {
          // Print SSID and RSSI for each network found
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(WiFi.SSID(i));
          Serial.print(" (");
          Serial.print(WiFi.RSSI(i));
          Serial.print(")");
          Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
          delay(10);
        }
    }
  }
}

/***************************** Filesystem **************************/

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readConfig(String filename) {
  JsonDocument testDocument;
	File configFile = LittleFS.open(filename);
	if (configFile)
	{
		Serial.println("opened config file");
		DeserializationError error = deserializeJson(testDocument, configFile);

		// Test if parsing succeeds.
		if (error)
		{
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(error.f_str());
			return;
		}

		Serial.println("deserializeJson ok");
		{
			Serial.println("Lese Daten aus Config - Datei");
			strcpy(tWeb_Config.wAP_SSID, testDocument["SSID"] | "NoWa");
			strcpy(tWeb_Config.wAP_Password, testDocument["Password"] | "12345678");
      strcpy(tWeb_Config.wBMP_Sensortype, testDocument["BMP"] | "0");
			Serial.println(tWeb_Config.wAP_SSID);
		}
		configFile.close();
		Serial.println("Config - Datei geschlossen");
	}

	else
	{
		Serial.println("failed to load json config");
	}
}


bool writeConfig(String json)
{
	Serial.println(json);

	Serial.println("neue Konfiguration speichern");

	File configFile = LittleFS.open("/config.json", FILE_WRITE);
	if (configFile)
	{
		Serial.println("Config - Datei öffnen");
		File configFile = LittleFS.open("/config.json", FILE_WRITE);
		if (configFile)
		{
			Serial.println("Config - Datei zum Schreiben geöffnet");
			JsonDocument testDocument;
			Serial.println("JSON - Daten übergeben");
			DeserializationError error = deserializeJson(testDocument, json);
			// Test if parsing succeeds.
			if (error)
			{
				Serial.print(F("deserializeJson() failed: "));
				Serial.println(error.f_str());
				// bei Memory - Fehler den <Wert> in StaticJsonDocument<200> testDocument; erhöhen
				return false;
			}
			Serial.println("Konfiguration schreiben...");
			serializeJson(testDocument, configFile);
			Serial.println("Konfiguration geschrieben...");

			// neue Config in Serial ausgeben zur Kontrolle
			serializeJsonPretty(testDocument, Serial);

			Serial.println("Config - Datei geschlossen");
			configFile.close();
		}
	}
	return true;
}
/***************************** I2C Bus **************************/

void I2C_scan(void){
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) 
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
      {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      String I2CAdress = String(address,HEX);
      sI2C_Status = "Device gefunden an 0x" + I2CAdress;
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      sI2C_Status = "Device Fehler";
      if (address<16) 
      {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
    sI2C_Status = "Nichts gefunden";
  }
  else {
    Serial.println("done\n");
  }
}

String sWifiStatus(int Status)
{
  switch(Status){
    case WL_IDLE_STATUS:return "Warten";
    case WL_NO_SSID_AVAIL:return "Keine SSID vorhanden";
    case WL_SCAN_COMPLETED:return "Scan komlett";
    case WL_CONNECTED:return "Verbunden";
    case WL_CONNECT_FAILED:return "Verbindung fehlerhaft";
    case WL_CONNECTION_LOST:return "Verbindung verloren";
    case WL_DISCONNECTED:return "Nicht verbunden";
    default:return "unbekannt";
  }
}

#endif   