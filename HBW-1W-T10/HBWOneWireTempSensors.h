/*
 * HBWOneWireTempSensors.h
 *
 *  Created on: 02.04.2019
 *      Author: loetmeister.de Vorlage: hglaser, thorsten@pferdekaemper.com, Hoschiq
 */

#ifndef HBWOneWireTempSensors_h
#define HBWOneWireTempSensors_

//DS18S20 Gerätecode 0x10
//DS18B20 Gerätecode 0x28
//DS1822  Gerätecode 0x22


#include "HBWired.h"
#include <OneWire.h>

#define DEBUG_OUTPUT   // extra debug output on serial/USB
//#define EXTRA_DEBUG_OUTPUT   // even more debug output


#define DEFAULT_TEMP -27315   // for unused channels
#define ERROR_TEMP -27314     // CRC error
#define OW_DEVICE_ADDRESS_SIZE 8   // fixed length for temp sensors
#define OW_POLL_FREQUENCY 1200  // read temperature every X milli seconds (not less than 900 ms! -> 750 ms conversion time @12 bit resolution)

// config of each sensor
struct hbw_config_onewire_temp {
  byte send_delta_temp;                  // Temperaturdifferenz, ab der gesendet wird
  byte offset;			                     // offset in m°C (-1.27..+1.27 °C)
  uint16_t send_min_interval;            // Minimum-Sendeintervall
  uint16_t send_max_interval;            // Maximum-Sendeintervall
  byte address[OW_DEVICE_ADDRESS_SIZE];  // 1-Wire-Adresse // TODO: add option to delete -> FEHM cmd?, XML?
};


// Class HBWOneWireTempSensors
class HBWOneWireTemp : public HBWChannel {
  public:
    HBWOneWireTemp(OneWire* _ow, hbw_config_onewire_temp* _config, uint32_t* _owLastReadTime, uint8_t* _owCurrentChannel, uint8_t _channels);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void afterReadConfig();
    static void sensorSearch(OneWire* ow, hbw_config_onewire_temp** _config, uint8_t channels, uint8_t address_start);
	
  private:
	  int16_t oneWireReadTemp();
	  void oneWireStartConversion();
    OneWire* ow {NULL};
    hbw_config_onewire_temp* config;
    uint8_t channelsTotal;
    uint8_t* owCurrentChannel;
    uint32_t* owLastReadTime;   // when was the onewiresensor (actually the bus) last prompted
    int16_t currentTemp;	// temperatures in m°C
    int16_t lastSentTemp;	// temperature measured on last send
    uint32_t lastSentTime;		// time of last send
    struct s_errorState {
      byte count:3;         // channel in error state if counted down to 0
      byte wasSend:1;       // flag to indicate if ERROR_TEMP was send
	    byte isAllZeros:1;    // indicate that current device read was blank (ususally happens when device gets disconnected)
      byte dummy:3;
    } errorState;
};

#endif
