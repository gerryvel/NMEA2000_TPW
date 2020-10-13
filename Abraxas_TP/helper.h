#include <stdio.h>
#include <time.h>
#include "configuration.h"
#include <Arduino.h>

void ShowTime()
{
	time_t now = time(NULL);
	struct tm tm_now;
	localtime_r(&now, &tm_now);
	char buff[100];
	strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S", &tm_now);
	printf("Zeit: %s\n", buff);
}

#define WEB_TITEL "ABRAXAS Wetterdaten"

String SendHTML(IPAddress AP_IP, IPAddress SELF_IP, IPAddress CL_IP, String C_SSID, float temperature, float pressure, float altitude, String msg1, double WDirection, double WSpeed, String strBoardInfo)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<meta http-equiv=\"refresh\" content=\"";
  ptr += PAGE_REFRESH;
  ptr += "\">\n";
  ptr +="<title>";
  ptr += WEB_TITEL; 
  ptr += "</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: left;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body >\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>";
  ptr += WEB_TITEL;
  ptr += "</h1>\n";
  ptr +="<p>Temperatur: ";
  ptr +=temperature;
  ptr +=" &deg;C</p>";
  ptr +="<p>Luftdruck: ";
  ptr +=pressure;
  ptr +=" Pa</p>";
  ptr +="<p>H&ouml;he: ";
  ptr +=altitude;
  ptr +=" Meter</p>";
  ptr +="</div>\n\n\n";
  ptr +="<div>";
  ptr += "<br>AP IP-Adresse: ";
  ptr += AP_IP.toString();
  ptr += "<br>Eigene IP-Adresse: ";
  ptr += SELF_IP.toString();
  ptr += "<br>Client IP-Adresse: ";
  ptr += CL_IP.toString();
  ptr += " an AP: ";
  ptr += C_SSID;
  ptr += "</div>";
  ptr += "<div>";
  ptr += "<br><b>NMEA0183 empf�ngt:</b><br>";
  ptr += "<br>Windrichtung: ";
  ptr += WDirection;
  ptr += "<br>Windgeschwindigkeit: ";
  ptr += WSpeed;
  ptr += "</div>";
  ptr += "<div>";
  ptr += "<br><b>NMEA2000 sendet:</b><br>";
  ptr += msg1;
  ptr +="</div>";
  ptr +="<div>";
  ptr += "<br><br><b>Informationen zum ESP32 - Board:</b><br><br>";
  ptr += strBoardInfo;
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
