#ifndef __WEB__H__
#define __WEB__H__

#include <arduino.h>
#include "BoardInfo.h"
#include "configuration.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "helper.h"

// Set web server port number to 80
AsyncWebServer server(80);
AsyncEventSource events("/events");

// Info Board for HTML-Output
BoardInfo boardInfo;
String sBoardInfo;

String processor(const String& var)
{
	if (var == "CONFIGPLACEHOLDER")
	{
		String buttons = "";
		buttons += "<form onSubmit = \"event.preventDefault(); formToJson(this);\">";
		buttons += "<p class=\"CInput\"><label>SSID </label><input type = \"text\" name = \"SSID\" value=\"";
		buttons += tAP_Config.wAP_SSID;
		buttons += "\"/></p>";
		buttons += "<p class=\"CInput\"><label>Password </label><input type = \"text\" name = \"Password\" value=\"";
		buttons += tAP_Config.wAP_Password;
		buttons += "\"/></p>";
		buttons += "<p><input type=\"submit\" value=\"Speichern\"></p>";
		buttons += "</form>";
		return buttons;
	}
	return String();
}


//Variables for website
String sCL_Status = sWifiStatus(WiFi.status());
String replaceVariable(const String& var){
	if (var == "sWDirection")return String(dVWR_WindDirectionM,1);
	if (var == "sWGaugeDirection")return String(dVWR_WindDirectionM, 1);
	if (var == "sWSpeed")return String(dVWR_WindSpeedkn,1);
	if (var == "sTemp")return String(fbmp_temperature, 1);
  if (var == "sPress")return String(fbmp_pressure/100, 0);
	if (var == "sBoardInfo")return sBoardInfo;
  if (var == "sFS_Space")return String(LittleFS.usedBytes());
	if (var == "sAP_IP")return WiFi.softAPIP().toString();
  if (var == "sAP_Clients")return String(WiFi.softAPgetStationNum());
  if (var == "sCL_Addr")return WiFi.localIP().toString();
  if (var == "sCL_Status")return String(sCL_Status);
  if (var == "sI2C_Status")return String(sI2C_Status);
  if (var == "sBMP_Status")return String(sBMP_Status);
  if (var == "sCL_SSID")return String(CL_SSID);
  if (var == "sCL_PASSWORD")return String(CL_PASSWORD);
  if (var == "CONFIGPLACEHOLDER")return processor(var);
  return "NoVariable";
}
void website(){
server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon.ico", "image/x-icon");
	});
server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/index.html", String(), false, replaceVariable);
	});
server.on("/system.html", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/system.html", String(), false, replaceVariable);
	});
server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/settings.html", String(), false, replaceVariable);
	});
server.on("/ueber.html", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/ueber.html", String(), false, replaceVariable);
	});
server.on("/gauge.min.js", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/gauge.min.js");
	});
server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/style.css", "text/css");
	});
server.on("/settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
	{
		int count = request->params();
		Serial.printf("Anzahl: %i\n", count);
		for (int i = 0;i < count;i++)
		{
			AsyncWebParameter* p = request->getParam(i);
			Serial.print("PWerte von der Internet - Seite: ");
			Serial.print("Param name: ");
			Serial.println(p->name());
			Serial.print("Param value: ");
			Serial.println(p->value());
			Serial.println("------");
			// p->value in die config schreiben
			writeConfig(p->value());
		}
		request->send(200, "text/plain", "Daten gespeichert");
	});
}

#endif