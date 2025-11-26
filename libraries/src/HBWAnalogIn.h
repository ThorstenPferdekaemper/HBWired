/*
 * HBWAnalogIn.h
 * 
 * analog input channel, max. 16 bit reading
 * 
 * www.loetmeister.de
 * 
 */

#ifndef HBWAnalogIn_h
#define HBWAnalogIn_h

#include <inttypes.h>
#include "HBWired.h"


//#define DEBUG_OUTPUT   // extra debug output on serial/USB

static const uint8_t SAMPLE_INTERVAL = 3;  // seconds * 3 samples
static const uint16_t DEFAULT_UPDATE_INTERVAL = 300;  // seconds

// address step 6
struct hbw_config_analog_in {
  uint8_t send_delta_value;       // Differenz des Messwerts, ab dem gesendet wird
  uint16_t send_max_interval;     // Maximum-Sendeintervall
  uint8_t send_min_interval;      // Minimum-Sendeintervall (factor 10, range 10 to 2540 seconds)
  uint8_t update_interval;        // factor 10, range 10 to 2540 seconds
  uint8_t dummy;
};


// Class HBWAnalogIn
class HBWAnalogIn : public HBWChannel {
  public:
    HBWAnalogIn(uint8_t _pin, hbw_config_analog_in* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void afterReadConfig();
    
  private:
    uint8_t pin;   // Pin
    hbw_config_analog_in* config;
    uint32_t lastActionTime;
    uint16_t nextActionDelay;
    uint16_t currentValue;  // store last result (average)
    uint16_t lastSendValue;  // result that has been send (average)
    uint32_t lastSentTime;  // last time of successful send
};

#endif
