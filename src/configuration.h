#ifndef __configuration__H__
#define __configuration__H__

#include <Arduino.h>

// Versionierung
#define Version "V2.2 vom 22.08.2024"  // Version

// Configuration N2k
#define ESP32_CAN_TX_PIN GPIO_NUM_4  // Set CAN TX port to 4 
#define ESP32_CAN_RX_PIN GPIO_NUM_5  // Set CAN RX port to 5
#define N2K_SOURCE 15
#define TempSendOffset 0 // Send time offsets
#define PressSendOffset 50 // Send time offsets
#define WindSendOffset 100 // Send time offsets
#define SlowDataUpdatePeriod 1000 // Time between CAN Messages sent

//Configuration Web Page 
#define PAGE_REFRESH 10 // x Sec.
#define WEB_TITEL "NMEA2000TPW"

//Configuration mit Webinterface
struct Web_Config
{
	char wAP_SSID[64];
	char wAP_Password[12];
};
Web_Config tAP_Config;

//Configuration AP 
#define HostName        "NMEA2000TPW"
#define AP_SSID         "NMEA2000TPW"  // SSID Name
#define AP_PASSWORD     "12345678"    // SSID Password - Set to NULL to have an open AP
const int   channel        = 10;                // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;             // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 4;                 // Maximum simultaneous connected clients on the AP

// Variables for WIFI-AP
IPAddress IP = IPAddress(192, 168, 15, 1);
IPAddress Gateway = IPAddress(192, 168, 15, 1);
IPAddress NMask = IPAddress(255, 255, 255, 0);
IPAddress AP_IP;
IPAddress CL_IP;
IPAddress SELF_IP;

//Configuration Client (Network Data Windsensor)
//#define 
String CL_SSID = "";					// Standard NoWa 
String CL_PASSWORD = "";			
int iSTA_on = 0;                            // Status STA-Mode
int bConnect_CL = 0;
bool bClientConnected = 0;

//Confuration Sensors bmp
#define BMP_SDA 21                      //Standard 21
#define BMP_SCL 22                      //Standard 22
#define SEALEVELPRESSURE_HPA (1013.25)  //1013.25
float fbmp_temperature = 0;
float fbmp_pressure = 0;
float fbmp_altitude = 0;
String sBMP_Status = "";
String sI2C_Status = "";

//Definitions Wind Data
float TwindDirection = 0.0;
float TwindSpeedkn = 0.0;
float TwindSpeedms = 0.0;

float MwindDirection = 0.0;
float MwindSpeedkn = 0.0;
float MwindSpeedms = 0.0;

//Definiton NMEA0183
double dMWV_WindAngle = 0;      // Relative Windrichtung in rad
double dMWV_WindSpeed = 0;      // Relative Windgeschwindigkeit
double dVWR_WindDirection = 0; // Absolute Windrichtung in rad
double dVWR_WindSpeedkn = 0;    // Absolute Windgeschwindigkeit in Knoten
double dVWR_WindSpeedms = 0;    // Absolute Windgeschwindigkeit in m/s
double dVWR_WindSpeedkm = 0;    // Absolute Windgeschwindigkeit in km/h
double dXDR_Kraengung = 0;

//Configuration NMEA0183
#define SERVER_HOST_NAME "192.168.5.1"		//"192.168.76.34"
#define TCP_PORT 6666						//6666
#define DNS_PORT 53

#endif  