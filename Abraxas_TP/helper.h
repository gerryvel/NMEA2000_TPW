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

void ShowTime(){
	time_t now = time(NULL);
	struct tm tm_now;
	localtime_r(&now, &tm_now);
	char buff[100];
	strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S", &tm_now);
	printf("Zeit: %s\n", buff);
}

#define WEB_TITEL "NMEA2000 TPW"

void freeHeapSpace(){
	static unsigned long last = millis();
	if (millis() - last > 5000) {
		last = millis();
		Serial.printf("\n[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
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
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
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
  }
  else {
    Serial.println("done\n");
  }
}

String sWifiStatus(int Status) // Status als String
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
