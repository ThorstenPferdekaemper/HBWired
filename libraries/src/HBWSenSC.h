/*
 * HBWSenSC.h
 * 
 * sensor/shutter contact
 * Query to this type of channel will return "contact_closed" or "contact_open" (boolean)
 * No peering. I-Message/notify on status change can be enabled by NOTIFY: on/off
 * 
 * www.loetmeister.de
 * 
 */

#ifndef HBWSenSC_h
#define HBWSenSC_h

#include <inttypes.h>
#include "HBWired.h"


//#define DEBUG_OUTPUT   // extra debug output on serial/USB


// config of each sensor channel, address step 1
struct hbw_config_senSC {
  uint8_t n_input_locked:1;   // +0.0    0=LOCKED, 1=UNLOCKED (default)
  uint8_t n_inverted:1;       // +0.1    0=inverted, 1=not inverted (default)
  uint8_t notify_disabled:1;  // +0.2    0=ENABLED, 1=DISABLED (default)
  uint8_t       :5;           // +0.3-7
  //uint8_t       :3;           // make DEBOUNCE_TIME configurable? (200ms steps, start at 0 = 200.. 7 = 1400)
};


// Class HBWSenSC
class HBWSenSC : public HBWChannel {
  public:
    HBWSenSC(uint8_t _pin, hbw_config_senSC* _config, boolean _activeHigh = false);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    
  private:
    uint8_t pin;   // Pin
    hbw_config_senSC* config;
    uint32_t keyPressedMillis;  // Zeit, zu der die Taste gedrueckt wurde (fuer's Entprellen)
    boolean currentValue;
    boolean initDone;
    boolean activeHigh;    // activeHigh=true -> input active high, else active low

    inline boolean readScInput() {
      boolean reading  = (digitalRead(pin) ^ !config->n_inverted);
      return (activeHigh ^ reading);
    }
	
	static const uint8_t DEBOUNCE_TIME = 88;  // ms
};

#endif
