// Analog input (ADC)

#ifndef HBWAnalogIn_h
#define HBWAnalogIn_h

#include <inttypes.h>
#include "HBWired.h"

// TODO: wahrscheinlich ist es besser, bei EEPROM-re-read
//       callbacks fuer die einzelnen Kanaele aufzurufen 
//       und den Kanaelen nur den Anfang "ihres" EEPROMs zu sagen
struct hbw_config_analog_in {
	uint8_t enabled:1;      // 0x0:1    0=disabled, 1=enabled
  uint8_t samples:3;
	uint8_t        :4;      // 0x0
	uint8_t sample_interval;
};


class HBWAnalogIn : public HBWChannel {
  public:
    HBWAnalogIn(uint8_t _pin, hbw_config_analog_in* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void afterReadConfig();
    
  private:
    uint8_t pin;
    hbw_config_analog_in* config;
    uint32_t lastReadTime;  // time of last ADC reading
    uint16_t nextReadDelay;
    
    uint16_t currentValue;
};

#endif
