
// Configuration N2k
#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Set CAN TX port to 5 
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Set CAN RX port to 4
#define N2K_SOURCE 15

//Configuration Refresh Page x Sec.
#define PAGE_REFRESH 10 

//Configuration AP 
#define AP_SSID      "ABRAXAS_TP"
#define AP_PASSWORD  "12345678"
#define HostName     "Abraxas_TP"

//Configuration Client
#define CL_SSID      "NoWa"
#define CL_PASSWORD  "12345678"

//Confuration Sensors
#define BMP_STA 21
#define BMP_SCL 22
float bmp280_temperature = 0;
float bmp280_pressure = 0;
float bmp280_altitude = 0;

//Definiton NMEA0183 MWV
double NMEA0183_WindDirectionT = 0;
double NMEA0183_WindDirectionM = 0;
double NMEA0183_WindSpeedM = 0;
double NMEA0183_WindAngle = 0;


//Configuration NMEA0183
#define SERVER_HOST_NAME "192.168.4.1"
#define TCP_PORT 6666
#define DNS_PORT 53