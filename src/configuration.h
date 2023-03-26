#ifndef __configuration__H__
#define __configuration__H__

// Configuration N2k
#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Set CAN TX port to 5 
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Set CAN RX port to 4
#define N2K_SOURCE 15

//Configuration Refresh Page x Sec.
#define PAGE_REFRESH 10 

//Configuration AP 
#define HostName        "NMEA2000TPW"
#define AP_SSID         "NMEA2000TPW"  // SSID Name
#define AP_PASSWORD     "12345678"    // SSID Password - Set to NULL to have an open AP
const int   channel        = 10;                // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;             // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 2;                 // Maximum simultaneous connected clients on the AP

// Variables for WIFI-AP
IPAddress IP = IPAddress(192, 168, 15, 1);
IPAddress Gateway = IPAddress(192, 168, 15, 1);
IPAddress NMask = IPAddress(255, 255, 255, 0);
IPAddress AP_IP;
IPAddress CL_IP;
IPAddress SELF_IP;

//Configuration Client (Network Data Windsensor)
#define CL_SSID      "NoWa"					//"ABRAXAS"
#define CL_PASSWORD  "12345678"				//"schmilka200"
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

//Definiton NMEA0183 MWV
double dMWV_WindDirectionT = 0;
double dMWV_WindSpeedM = 0;
double dVWR_WindDirectionM = 0;
double dVWR_WindAngle = 0;
double dVWR_WindSpeedkn = 0;
double dVWR_WindSpeedms = 0;

//Configuration NMEA0183
#define SERVER_HOST_NAME "192.168.4.1"		//"192.168.76.34"
#define TCP_PORT 6666						//6666
#define DNS_PORT 53

#endif  