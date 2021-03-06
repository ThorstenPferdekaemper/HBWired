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

#define DEBUG_OUTPUT   // extra debug output on serial/USB
//#define EXTRA_DEBUG_OUTPUT   // even more debug output

static const uint32_t OW_POLL_FREQUENCY = 1200;  // read temperature every X milli seconds (not less than 900 ms! -> 750 ms conversion time @12 bit resolution)
static const int16_t DEFAULT_TEMP = -27315;   // for unused channels
static const int16_t ERROR_TEMP = -27314;     // CRC or read error
#define OW_DEVICE_ADDRESS_SIZE 8   // fixed length for temp sensors address

#define OW_DEVICE_ERROR_COUNT 3  // sensor in error state if counted down to 0. Decremented on every failed read or CRC error

#define OW_CHAN_INIT 0xFF

// config of each sensor, address step 14
struct hbw_config_onewire_temp {
  uint8_t send_delta_temp;                  // Temperaturdifferenz, ab der gesendet wird
  uint8_t offset;                           // offset in c°C (-1.27..+1.27 °C)
  uint16_t send_min_interval;            // Minimum-Sendeintervall
  uint16_t send_max_interval;            // Maximum-Sendeintervall
  uint8_t address[OW_DEVICE_ADDRESS_SIZE];  // 1-Wire-Adresse
  // TODO: check if address can moved to seperate array, to only keep pointer here (ow_addr* ..?/ uint8_t* address = (uint8_t*)&ow_addr...; uint8_t ow_addr[8])
};


// Class HBWOneWireTempSensors
class HBWOneWireTemp : public HBWChannel {
  public:
    HBWOneWireTemp(OneWire* _ow, hbw_config_onewire_temp* _config, uint32_t* _owLastReadTime, uint8_t* _owCurrentChannel);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void afterReadConfig();
    static void sensorSearch(OneWire* ow, hbw_config_onewire_temp** _config, uint8_t channels, uint8_t address_start);

    enum TempSensor_State {
      ACTION_START_CONVERSION = 0,
      ACTION_READ_TEMP
    };


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
    uint8_t errorCount;    // channel/sensor in error state if counted down to 0. Decremented on every failed read
    uint8_t action;        // current action: measure, read temp
    boolean errorWasSend;  // flag to indicate if ERROR_TEMP was send
    boolean isAllZeros;    // indicate that current device read was blank (usually happens when device gets disconnected)
	
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
