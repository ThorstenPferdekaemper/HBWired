/*
 * HBWAnalogPow.cpp
 * 
 * analog power meter input channel, using current transformers connected to ADC
 * 
 * www.loetmeister.de
 * 
 */

#ifndef HBWAnalogPow_h
#define HBWAnalogPow_h

#include <inttypes.h>
#include "HBWired.h"


//#define DEBUG_OUTPUT   // extra debug output on serial/USB

#define SAMPLE_INTERVAL 2  // seconds
#define UPDATE_INTERVAL 133  // seconds

#define CENTRE_VALUE 511  // sin wave centre


struct hbw_config_analogPow_in {
  uint8_t input_disabled:1;     // +0:0   1=DISABLED (default), 0=ENABLED
  uint8_t notify_disabled:1;   // +0:1   1=DISABLED (default), 0=ENABLED
  uint8_t        :6;      // 0x00:2-7
  uint8_t update_interval;
};


// Class HBWAnalogPow
class HBWAnalogPow : public HBWChannel {
  public:
    HBWAnalogPow(uint8_t _pin, hbw_config_analogPow_in* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
//    virtual void afterReadConfig();
    
  private:
    uint8_t pin;   // Pin
    hbw_config_analogPow_in* config;
    uint32_t lastActionTime;
    uint8_t nextActionDelay;
    uint16_t currentValue;  // store last result (average)
};

#endif
