
// Configuration N2k
#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Set CAN TX port to 5 
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Set CAN RX port to 4
#define N2K_SOURCE 15

//Configuration Refresh Page x Sec.
#define PAGE_REFRESH 10 

//Configuration AP 
const char* HostName       = "NMEA2000TPW";
const char* AP_SSID        = "NMEA2000TPW";  // SSID Name
const char* AP_PASSWORD    = "12345678";    // SSID Password - Set to NULL to have an open AP
const int   channel        = 10;                // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;             // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 2;                 // Maximum simultaneous connected clients on the AP

//Configuration Client (Network Data Windsensor)
#define CL_SSID      "NoWa"					
#define CL_PASSWORD  "12345678"				
int iSTA_on = 0;                            // Status STA-Mode
int bConnect_CL = 0;
bool bClientConnected = 0;

//Confuration Sensors
#define BMP_STA 21
#define BMP_SCL 22
float bmp280_temperature = 0;
float bmp280_pressure = 0;
float bmp280_altitude = 0;

//Definiton NMEA0183 MWV
double MWV_WindDirectionT = 0;
double MWV_WindSpeedM = 0;
double VWR_WindDirectionM = 0;
double VWR_WindAngle = 0;
double VWR_WindSpeedkn = 0;
double VWR_WindSpeedms = 0;

//Configuration NMEA0183
#define SERVER_HOST_NAME "192.168.4.1"		//"192.168.76.34"
#define TCP_PORT 6666						//6666
#define DNS_PORT 53

