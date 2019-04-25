/*
 * HBWOneWireTempSensors.h
 *
 *  Created on: 02.04.2019
 *      Author: loetmeister.de Vorlage: hglaser, thorsten@pferdekaemper.com, Hoschiq
 */

#ifndef HBWOneWireTempSensors_h
#define HBWOneWireTempSensors_h

/* Supported sensors */
//DS18S20 Gerätecode 0x10
//DS18B20 Gerätecode 0x28
//DS1822 Gerätecode 0x22


#include "HBWired.h"
#include <OneWire.h>

//#define DEBUG_OUTPUT   // extra debug output on serial/USB
//#define EXTRA_DEBUG_OUTPUT   // even more debug output

#define OW_POLL_FREQUENCY 1200  // read temperature every X milli seconds (not less than 900 ms! -> 750 ms conversion time @12 bit resolution)
#define DEFAULT_TEMP -27315   // for unused channels
#define ERROR_TEMP -27314     // CRC error
#define OW_DEVICE_ADDRESS_SIZE 8   // fixed length for temp sensors


#define ACTION_READ_TEMP 1
#define ACTION_START_CONVERSION 0


// config of each sensor
struct hbw_config_onewire_temp {
  byte send_delta_temp;                  // Temperaturdifferenz, ab der gesendet wird
  byte offset;                           // offset in m°C (-1.27..+1.27 °C)
  uint16_t send_min_interval;            // Minimum-Sendeintervall
  uint16_t send_max_interval;            // Maximum-Sendeintervall
  byte address[OW_DEVICE_ADDRESS_SIZE];  // 1-Wire-Adresse
};


// Class HBWOneWireTempSensors
class HBWOneWireTemp : public HBWChannel {
  public:
    HBWOneWireTemp(OneWire* _ow, hbw_config_onewire_temp* _config, uint32_t* _owLastReadTime, uint8_t* _owCurrentChannel);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void afterReadConfig();
    static void sensorSearch(OneWire* ow, hbw_config_onewire_temp** _config, uint8_t channels, uint8_t address_start);

  private:
    int16_t oneWireReadTemp();
    void oneWireStartConversion();
    OneWire* ow {NULL};
    hbw_config_onewire_temp* config;
    uint8_t* owCurrentChannel;    // channel currently running conversion - can only be one at a time
    uint32_t* owLastReadTime;   // when was the onewire sensor (actually the bus) last prompted
    int16_t currentTemp;	// temperatures in m°C
    int16_t lastSentTemp;	// temperature measured on last send
    uint32_t lastSentTime;		// time of last send
    struct s_state {
      byte errorCount:3;         // channel in error state if counted down to 0
      byte errorWasSend:1;       // flag to indicate if ERROR_TEMP was send
      byte isAllZeros:1;    // indicate that current device read was blank (usually happens when device gets disconnected)
      byte action:2;        // current action: measure, read temp
      byte dummy:1;
    } state;
    static bool deviceInvalidOrEmptyID(uint8_t deviceType);
    static const uint8_t DS18S20_ID = 0x10;
    static const uint8_t DS1822_ID = 0x22;
    static const uint8_t DS18B20_ID = 0x28;
};


// macro to print OneWire address by hbwdebug (just used to keep the main code a bit more readable....)
inline void m_hbwdebug_ow_address(uint8_t* address) {
  for (uint8_t i = 0; i < OW_DEVICE_ADDRESS_SIZE; i++) {
    hbwdebughex(address[i]);
  }
};

#endif
