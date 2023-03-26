#include "BoardInfo.h"
#include <esp_system.h>
#include <esp_spi_flash.h>

 #ifdef __cplusplus
  extern "C" {
 #endif
 
  uint8_t temprature_sens_read();
 
#ifdef __cplusplus
}
#endif
 
uint8_t temprature_sens_read();

#define BUF 255

BoardInfo::BoardInfo()
{
    // Konstruktor der Klasse
    // ChipID auslesen
    //The chip ID is essentially its MAC address(length: 6 bytes).
    m_chipid = 0;
    m_chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
    // Chip - Info auslesen
    esp_chip_info(&m_chipinfo);
}

void BoardInfo::ShowChipID()
{
    if (m_chipid != 0)
    {
    	Serial.printf("ESP32 Chip ID = %04X",(uint16_t)(m_chipid>>32));        //print High 2 bytes
    	Serial.printf("%08X\n",(uint32_t)m_chipid);                            //print Low 4bytes.
    }
    else
    {
        // Fehler beim Lesen der ID....
    	Serial.println("ESP32 Chip ID konnte nicht ausgelesen werden");
    }
}

String BoardInfo::ShowChipIDtoString()
{
    String msg;
    if (m_chipid != 0)
    {
        char string1[BUF];
        sprintf(string1, "ESP32 Chip ID = %04X%08X<br>",(uint16_t)(m_chipid>>32),(uint32_t)m_chipid);
        msg = (char*)string1;
        msg += "<br>";
        sprintf(string1, "%d CPU - Kerne<br>WLAN: %s<br>Bluetooth: %s%s",
            m_chipinfo.cores,
            (m_chipinfo.features & CHIP_FEATURE_WIFI_BGN) ? "2.4GHz" : "nicht vorhanden",
            (m_chipinfo.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (m_chipinfo.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
        msg += (char*)string1;
        msg += "<br>";
        sprintf(string1, "Silicon revision: %d", m_chipinfo.revision);
        msg += (char*)string1;  
        msg += "<br>"; 
        sprintf(string1, "%s Speicher %dMB", (m_chipinfo.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external", 
                                              spi_flash_get_chip_size() / (1024 * 1024));

        msg += (char*)string1;
        msg += "<br>";
        sprintf(string1, "Freier Speicher: %d bytes", ESP.getFreeHeap());
        msg += (char*)string1;
        msg += "<br>";
        sprintf(string1, "Min freier Speicher: %d bytes", esp_get_minimum_free_heap_size());
        msg += (char*)string1;
        msg += "<br>";
    }
    else
    {
        // Fehler beim Lesen der ID....
    	msg = "ESP32 Chip ID konnte nicht ausgelesen werden";
    }
    return msg;
}

void BoardInfo::ShowChipInfo()
{
    // Infos zum Board
    Serial.printf("Das ist ein Chip mit %d CPU - Kernen\nWLAN: %s\nBluetooth: %s%s\n",
            m_chipinfo.cores,
            (m_chipinfo.features & CHIP_FEATURE_WIFI_BGN) ? "2.4GHz" : "nicht vorhanden",
            (m_chipinfo.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (m_chipinfo.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    Serial.printf("Silicon revision %d\n", m_chipinfo.revision);

    Serial.printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (m_chipinfo.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    Serial.printf("(Freier Speicher: %d bytes)\n", esp_get_free_heap_size());
    Serial.printf("Freier Speicher: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Minimum freier Speicher: %d bytes\n", esp_get_minimum_free_heap_size());
}

void BoardInfo::ShowChipTemperature()
{
    uint8_t temp_farenheit;
    float temp_celsius;
    temp_farenheit = temprature_sens_read();
    if (128 == temp_farenheit)
    {
        Serial.println("Kein Temperatur - Sensor vorhanden.");
        return;
    }
    temp_celsius = ( temp_farenheit - 32 ) / 1.8;
    Serial.printf("Temperatur Board: %i Fahrenheit\n", temp_farenheit);
    Serial.printf("Temperatur Board: %.1f °C\n", temp_celsius);
}
