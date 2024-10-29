#ifndef _NMEA0183_H_
#define _NMEA0183_H_

#include <Arduino.h>
#include "configuration.h"

// NMEA0183
#include "NMEA0183.h"
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>

// N2k
#include <N2kMsg.h>
#include <N2kMessages.h>

// NMEA 0183 Stream
tNMEA0183 NMEA0183;  // NMEA stream for NMEA0183 receiving
WiFiClient nmeaclient;
tNMEA0183WindReference Reference;
char MWVSpeedUnit = '?';
char buffer[128]; // Puffer zur Speicherung der empfangenen Daten
int bufferIndex = 0;

void NMEA0183_read()
{
  if (nmeaclient.connected()) {
    while (nmeaclient.available()) 
    {
      char c = nmeaclient.read();
      Serial.write(c);

      if (c == '\n' || bufferIndex >= sizeof(buffer) - 1) 
      {
        buffer[bufferIndex] = '\0'; // Abschluss der Zeichenkette

        // Überprüfen, ob es sich um eine WST-Nachricht handelt              
        if (strstr(buffer, "WST") != NULL) 
         {
          // $PWWST,C,0,x.x,A*hh<CR><LF>
          sscanf(buffer, "$%*[^,],%*[^,],%*[^,],%f,", &fWindSensorTemp);  
            // Speichern Daten in Variablen
          Serial.printf("Wind Sensor Temperatur: %f °\n", fWindSensorTemp);

          dWindSensorTemp = fWindSensorTemp;
        }

        // Puffer zurücksetzen
        bufferIndex = 0;
      } else {
        buffer[bufferIndex++] = c;
      }
    }
  }  
}

void NMEA0183_reconnect(){

 if (!nmeaclient.connected()){
        nmeaclient.stop();
        // Versuchen, die Verbindung wiederherzustellen
    if (nmeaclient.connect(SERVER_HOST_NAME, TCP_PORT)) {
      Serial.println("Connected to NMEA-Server");
    } else {
      Serial.println("Connection to NMEA-Server failed");
    }
  }
}

double WindAngle;
double WindSpeed;

void HandleMWV(const tNMEA0183Msg &NMEA0183Msg) {  
  tNMEA0183WindReference Reference;
  tN2kWindReference WindReference;
  tN2kMsg N2kMsg;
  
  Serial.println("MWV Message: ");
  
  // Parse MWV message (WindSpeed is in m/s !!!)
  if (!NMEA0183ParseMWV_nc(NMEA0183Msg, WindAngle, Reference, WindSpeed)) return;

  // Read/Set wind reference
  if(Reference == NMEA0183Wind_True) {
    WindReference =  N2kWind_True_boat;
  } else {
    WindReference =  N2kWind_Apparent;
  }

  Serial.printf("Angle=%4.1f°, Speeed=%3.1f kn, Reference=%d\n", WindAngle, msToKnots(WindSpeed), Reference);
    // Create NMEA2000 message
  //SetN2kWindSpeed(N2kMsg, 0, WindSpeed, DegToRad(WindAngle), WindReference);
  
  dMWV_WindSpeed = WindSpeed;
  dMWV_WindAngle = DegToRad(WindAngle);
  dMWV_Reference = WindReference;
}

void NMEA0183_ParseMessages() {

  tNMEA0183Msg NMEA0183Msg;  // Create message container
    
  if (!NMEA0183.GetMessage(NMEA0183Msg)){
      Serial.println("No NMEA0183 Message !");      
  } 
  else {
   Serial.println("yeah… new Message !");
}
  if (NMEA0183Msg.IsMessageCode("MWV")){
      HandleMWV(NMEA0183Msg);
  }
}     

#endif   