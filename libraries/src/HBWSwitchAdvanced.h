/* 
* HBWSwitchAdvanced
*
* Als Alternative zu HBWSwitch & HBWLinkSwitchSimple sind mit
* HBWSwitchAdvanced & HBWLinkSwitchAdvanced folgende Funktionen möglich:
* Peering mit TOGGLE, TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, onTime,
* offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung).
*
* http://loetmeister.de/Elektronik/homematic/
*
*/

#ifndef HBWSwitchAdvanced_h
#define HBWSwitchAdvanced_h

#include <inttypes.h>
#include "HBWlibStateMachine.h"
#include "HBWired.h"


//#define NO_DEBUG_OUTPUT   // disable debug output on serial/USB


// peering/link values must match the XML/EEPROM values!
#define JT_ONDELAY 0x00
#define JT_ON 0x01
#define JT_OFFDELAY 0x02
#define JT_OFF 0x03
#define JT_NO_JUMP_IGNORE_COMMAND 0x04


// TODO: wahrscheinlich ist es besser, bei EEPROM-re-read
//       callbacks fuer die einzelnen Kanaele aufzurufen 
//       und den Kanaelen nur den Anfang "ihres" EEPROMs zu sagen
struct hbw_config_switch {
  uint8_t logging:1;              // 0x0000
  uint8_t output_unlocked:1;      // 0x07:1    0=LOCKED, 1=UNLOCKED
  uint8_t n_inverted:1;           // 0x07:2    0=inverted, 1=not inverted (device reset will set to 1!)
  uint8_t        :5;              // 0x0000
  uint8_t dummy;
};


class HBWSwitchAdvanced : public HBWChannel {
  public:
    HBWSwitchAdvanced(uint8_t _pin, hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);   
    virtual void loop(HBWDevice*, uint8_t channel);   
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();
    
  private:
    uint8_t pin;
    hbw_config_switch* config; // logging
    HBWlibStateMachine StateMachine;

    void setOutput(HBWDevice* device, uint8_t const * const data);
};

#endif
