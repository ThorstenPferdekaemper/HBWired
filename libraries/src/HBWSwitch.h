// Switches

#ifndef HBWSwitch_h
#define HBWSwitch_h

#include <inttypes.h>
#include "HBWired.h"

// TODO: wahrscheinlich ist es besser, bei EEPROM-re-read
//       callbacks fuer die einzelnen Kanaele aufzurufen 
//       und den Kanaelen nur den Anfang "ihres" EEPROMs zu sagen
// config of one channel, address step 2
struct hbw_config_switch {
	uint8_t logging:1;              // 0x0000
	uint8_t output_unlocked:1;      // 0x07:1    0=LOCKED, 1=UNLOCKED
	uint8_t n_inverted:1;           // 0x07:2    0=inverted, 1=not inverted (device reset will set to 1!)
	uint8_t        :5;              // 0x0000
	uint8_t dummy;
};


class HBWSwitch : public HBWChannel {
  public:
    HBWSwitch(uint8_t _pin, hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);   
    virtual void loop(HBWDevice*, uint8_t channel);   
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();

  private:
    uint8_t pin;
    uint8_t currentState;    // keep track of logical state, not real IO
    hbw_config_switch* config; // logging
};

#endif