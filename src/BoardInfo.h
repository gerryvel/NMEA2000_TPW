#ifndef _Boardinfo_H_
#define _Boardinfo_H_


#include <Arduino.h>

class BoardInfo
{
public:
    BoardInfo();

    void ShowChipID();
    void ShowChipInfo();
    void ShowChipTemperature();

    String ShowChipIDtoString();
    
protected:
    uint64_t m_chipid; 
    esp_chip_info_t m_chipinfo; 
};

#endif