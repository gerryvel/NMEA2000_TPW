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
			Serial.println("Read data from config file");
			strcpy(tWeb_Config.wAP_SSID, testDocument["SSID"] | "NoWa");
			strcpy(tWeb_Config.wAP_Password, testDocument["Password"] | "12345678");
      strcpy(tWeb_Config.wBMP_Sensortype, testDocument["BMP"] | "0");
		}
		configFile.close();
		Serial.println("Config file closed");
	}

	else
	{
		Serial.println("failed to load json config");
	}
}


bool writeConfig(String json)
{
	Serial.println(json);

	Serial.println("Saving new configuration");

	File configFile = LittleFS.open("/config.json", FILE_WRITE);
	if (configFile)
	{
		Serial.println("Opening config file");
		File configFile = LittleFS.open("/config.json", FILE_WRITE);
		if (configFile)
		{
			Serial.println("Config file opened for writing");
			JsonDocument testDocument;
			Serial.println("Passing JSON data");
			DeserializationError error = deserializeJson(testDocument, json);
			// Test if parsing succeeds.
			if (error)
			{
				Serial.print(F("deserializeJson() failed: "));
				Serial.println(error.f_str());
				// bei Memory - Fehler den <Wert> in StaticJsonDocument<200> testDocument; erhöhen
				return false;
			}
			Serial.println("Writing configuration...");
			serializeJson(testDocument, configFile);
			Serial.println("Configuration written...");

			// neue Config in Serial ausgeben zur Kontrolle
			serializeJsonPretty(testDocument, Serial);

			Serial.println("Config file closed");
			configFile.close();
		}
	}
	return true;
}
/***************************** I2C Bus **************************/

bool readI2CRegister(uint8_t address, uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(address, static_cast<uint8_t>(1)) != 1) {
    return false;
  }
  value = Wire.read();
  return true;
}

int detectSensorType(uint8_t address, uint8_t &chipId) {
  if (readI2CRegister(address, 0x00, chipId) &&
      chipId == 0x50) {
    return SENSOR_BMP3XX;
  }
  if (readI2CRegister(address, 0xD0, chipId)) {
    if (chipId == 0x58) {
      return SENSOR_BMP280;
    }
    if (chipId == 0x60) {
      return SENSOR_BME280;
    }
  }
  return SENSOR_UNKNOWN;
}

const char *sensorTypeName(int sensorType) {
  switch (sensorType) {
    case SENSOR_BMP280: return "BMP280";
    case SENSOR_BMP3XX: return "BMP388";
    case SENSOR_BME280: return "BME280";
    default: return "Unbekannt";
  }
}

void I2C_scan(void){
  byte error, address;
  int nDevices = 0;
  int detectedSensors = 0;
  int detectedType = SENSOR_UNKNOWN;
  uint8_t detectedAddress = 0;
  uint8_t detectedChipId = 0;
  String foundAddresses = "";
  SensorI2CAddress = 0;
  sensorReady = false;

  Serial.println("Scanning...");
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      if (foundAddresses.length() > 0) {
        foundAddresses += ", ";
      }
      foundAddresses += "0x" + String(address, HEX);
      nDevices++;
      if (address == 0x76 || address == 0x77) {
        uint8_t chipId = 0;
        const int sensorType = detectSensorType(address, chipId);
        if (sensorType != SENSOR_UNKNOWN) {
          Serial.printf("Supported sensor chip ID 0x%02X at address 0x%02X\n",
                        chipId, address);
          detectedSensors++;
          detectedType = sensorType;
          detectedAddress = address;
          detectedChipId = chipId;
        }
      }
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }
  }

  if (detectedSensors == 1) {
    Sensortyp = detectedType;
    SensorI2CAddress = detectedAddress;
    const char *sensorName = Sensortyp == SENSOR_BMP280 ? "BMP280" :
                             Sensortyp == SENSOR_BMP3XX && detectedChipId == 0x50 ?
                               "BMP388" :
                             Sensortyp == SENSOR_BMP3XX ? "BMP3xx" : "BME280";
    sBMP = sensorName;
    sI2C_Status = String("Sensor ") + sensorName + " an 0x" +
                  String(SensorI2CAddress, HEX) + " (Chip-ID 0x" +
                  String(detectedChipId, HEX) + ")";
    sBMP_Status = "Sensor erkannt; Initialisierung ausstehend";
    Serial.println(sI2C_Status);
  } else if (detectedSensors > 1) {
    sBMP = String(sensorTypeName(Sensortyp)) + " (Konfiguration)";
    sBMP_Status = "Mehrere Sensoren; automatische Auswahl abgebrochen";
    sI2C_Status = "Mehrere unterstützte Sensoren gefunden; automatische Auswahl abgebrochen";
    Serial.println(sI2C_Status);
  } else if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
    sI2C_Status = "Nothing found";
    sBMP = String(sensorTypeName(Sensortyp)) + " (Konfiguration)";
    sBMP_Status = "Kein Sensor erkannt; Webkonfiguration bleibt ausgewählt";
  }
  else {
    sI2C_Status = "I2C-Geräte: " + foundAddresses +
                  "; kein unterstützter Sensor erkannt";
    sBMP = String(sensorTypeName(Sensortyp)) + " (Konfiguration)";
    sBMP_Status = "Sensortyp unbekannt; Webkonfiguration bleibt ausgewählt";
    Serial.println(sI2C_Status);
  }
}

String sWifiStatus(int Status)
{
  switch(Status){
    case WL_IDLE_STATUS:return "Warten"; break;
    case WL_NO_SSID_AVAIL:return "Keine SSID vorhanden"; break;
    case WL_SCAN_COMPLETED:return "Scan komlett"; break;
    case WL_CONNECTED:return "Verbunden"; break;
    case WL_CONNECT_FAILED:return "Verbindung fehlerhaft"; break;
    case WL_CONNECTION_LOST:return "Verbindung verloren"; break;
    case WL_DISCONNECTED:return "Nicht verbunden"; break;
    default:return "unbekannt";
  }
}

#endif   