#ifndef HBWSenSC_h
#define HBWSenSC_h

#include <inttypes.h>
#include "HBWired.h"


//#define DEBUG_OUTPUT   // extra debug output on serial/USB

#define DEBOUNCE_TIME 885  // ms


struct hbw_config_senSC {
  uint8_t n_input_locked:1;   // 0x07:0    0=LOCKED, 1=UNLOCKED (default)
  uint8_t n_inverted:1;       // 0x07:1    0=inverted, 1=not inverted (default)
  uint8_t notify_disabled:1;         // 0x07:2    0=ENABLED, 1=DISABLED (default)
  uint8_t       :5;           // 0x07:3-7
};


// Class HBWSenSC
class HBWSenSC : public HBWChannel {
  public:
    HBWSenSC(uint8_t _pin, hbw_config_senSC* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void afterReadConfig();
    
  private:
    uint8_t pin;   // Pin
    hbw_config_senSC* config;
    uint32_t keyPressedMillis;  // Zeit, zu der die Taste gedrueckt wurde (fuer's Entprellen)
    boolean currentValue;
    boolean currentState;

    uint16_t nextFeedbackDelay;
    uint32_t lastFeedbackTime;
};

#endif
