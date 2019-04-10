/*
 * HBWOneWireTempSensors.h
 *
 *  Created on: 02.04.2019
 *      Author: loetmeister.de Vorlage: hglaser, thorsten@pferdekaemper.com, Hoschiq
 */

#ifndef HBWOneWireTempSensors_h
#define HBWOneWireTempSensors_

// #include <inttypes.h>
#include "HBWired.h"
#include <OneWire.h>


#define DEBUG_OUTPUT   // extra debug output on serial/USB
//#define EXTRA_DEBUG_OUTPUT

#define DEFAULT_TEMP -27315   // for unused channels
#define ERROR_TEMP -27314     // CRC error
#define OW_ADDRESS_SIZE 8   // fixed length for temp sensors
#define OW_POLL_FREQUENCY 1200  // read temperature every X milli seconds (not less than 1000 ms!)

// config of each sensor
struct hbw_config_onewire_temp {
  byte send_delta_temp;         // 0x0D      0x1B // Temperaturdifferenz, ab der gesendet wird
  byte offset;			// 0x0E      0x1C
  uint16_t send_min_interval;   // 0x0F-0x10 0x1D-0x1E // Minimum-Sendeintervall
  uint16_t send_max_interval;   // 0x11-0X12 0x1F-0x20 // Maximum-Sendeintervall
  byte address[OW_ADDRESS_SIZE];              // 0x13-0x1A 0x21-0x28 // 1-Wire-Adresse // TODO: add option to delete -> FEHM cmd?, XML?
};


// Class HBWOneWireTempSensors
class HBWOneWireTemp : public HBWChannel {
  public:
    HBWOneWireTemp(OneWire* _ow, hbw_config_onewire_temp* _config, uint32_t* _owLastReadTime, uint8_t* _owCurrentChannel, uint8_t _channels);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void afterReadConfig();
    static void sensorSearch(OneWire* ow, hbw_config_onewire_temp** _config, uint8_t channels, uint8_t address_start, uint8_t address_step);
	
  private:
	  int16_t oneWireReadTemp();
	  void oneWireStartConversion();
    OneWire* ow {NULL};
    hbw_config_onewire_temp* config;
    uint8_t channelsTotal;
//    uint8_t state;
    //uint16_t nextFeedbackDelay;
    // uint32_t lastFeedbackTime;
    uint8_t* owCurrentChannel;
    //uint32_t lastReadTime;		// when was the onewiresensor last prompted // (normaly every 5 Seconds)
    uint32_t* owLastReadTime;
    int16_t currentTemp;	// temperatures in m°C
    int16_t lastSentTemp;	// temperature measured on last send
    uint32_t lastSentTime;		// time of last send
};

#endif
