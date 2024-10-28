#ifndef _NMEA0183_H_
#define _NMEA0183_H_

#include <Arduino.h>
#include "configuration.h"

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
    while (nmeaclient.available()) {
      char c = nmeaclient.read();
      Serial.write(c);

      if (c == '\n' || bufferIndex >= sizeof(buffer) - 1) {
        buffer[bufferIndex] = '\0'; // Abschluss der Zeichenkette

        // Überprüfen, ob es sich um eine VWR-Nachricht handelt
        if (strstr(buffer, "VWR") != NULL) {
          // Beispiel: "$IIVWR,090.0,R,005.5,N,002.8,M,009.0,K*50" oder $WIVWR,30.76,L,4.33,N,2.23,M,8.03,K*77
       
          sscanf(buffer, "$%*[^,],%f,%*[^,],%f,N,%f,M,", &TwindDirection, &TwindSpeedkn, &TwindSpeedms);    
            // Speichern Daten in Variablen
          Serial.printf("VWR Windrichtung: %f °\n", TwindDirection);
          Serial.printf("VWR Windgeschwindigkeit: %f kn\n", TwindSpeedkn);
          Serial.printf("VWR Windgeschwindigkeit. %f m/s\n\n", TwindSpeedms);
          dVWR_WindDirection = DegToRad(TwindDirection);
          dVWR_WindSpeedkn = TwindSpeedkn;
          dVWR_WindSpeedms = TwindSpeedms;

        }       
        else if (strstr(buffer, "MWV") != NULL) {
          // $WIMWV,333.33,T,0.00,K,A*23
          sscanf(buffer, "$%*[^,],%f,%c,%f,%c,", &MwindDirection, &Reference, &MwindSpeed, &MWVSpeedUnit);    //sscanf(buffer, "$%*[^,],%f,%[^,],%f,K,", &MwindDirection, &MwindSpeedkn); 
            // Speichern Daten in Variablen
          Serial.printf("MWV Windrichtung: %f ° %s\n", MwindDirection, Reference);
          Serial.printf("MWV Windgeschwindigkeit: %f %c\n\n", MwindSpeed, MWVSpeedUnit);

            //if (MwindDirection > 180.1)    // Windrichtung 0- +180 STB und 0 - -180 BB
            //MwindDirection = MwindDirection - 360.0;

          dMWV_WindAngle = DegToRad(MwindDirection);
          dMWV_WindSpeed = MwindSpeed;
        }

        // Puffer zurücksetzen
        bufferIndex = 0;
      } else {
        buffer[bufferIndex++] = c;
      }
    }
  } else {
    Serial.println("Verbindung zum NMEA-Server verloren");
    nmeaclient.stop();

    // Versuchen, die Verbindung wiederherzustellen
    if (nmeaclient.connect(SERVER_HOST_NAME, TCP_PORT)) {
      Serial.println("Verbunden mit NMEA-Server");
    } else {
      Serial.println("Verbindung zum NMEA-Server fehlgeschlagen");
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

void HandleMWV(const tNMEA0183Msg &NMEA0183Msg) {
  double WindAngle;
  tNMEA0183WindReference Reference;
  tN2kWindReference WindReference;
  double WindSpeed;
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