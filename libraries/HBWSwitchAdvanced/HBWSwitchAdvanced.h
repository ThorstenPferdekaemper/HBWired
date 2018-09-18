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
#include "HBWired.h"


//#define NO_DEBUG_OUTPUT   // disable debug output on serial/USB

// peering/link values must match the XML/EEPROM values!
#define JT_ONDELAY 0x00
#define JT_ON 0x01
#define JT_OFFDELAY 0x02
#define JT_OFF 0x03
#define JT_NO_JUMP_IGNORE_COMMAND 0x04
#define ON_TIME_ABSOLUTE 0x0A
#define OFF_TIME_ABSOLUTE 0x0B
#define ON_TIME_MINIMAL 0x0C
#define OFF_TIME_MINIMAL 0x0D
#define UNKNOWN_STATE 0xFF
#define FORCE_STATE_CHANGE 0xFE



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
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending

    // set from links/peering (implements state machine)
    void setOutput(HBWDevice* device, uint8_t const * const data);
    uint8_t getNextState(uint8_t bitshift);
    inline uint32_t convertTime(uint8_t timeValue);
    uint8_t actiontype;
    uint8_t onDelayTime;
    uint8_t onTime;
    uint8_t offDelayTime;
    uint8_t offTime;
    union { uint16_t WORD;
      uint8_t jt_hi_low[2];
    } jumpTargets;
    boolean stateTimerRunning;
    uint8_t currentState;
    uint8_t nextState;
    unsigned long stateChangeWaitTime;
    unsigned long lastStateChangeTime;
    uint8_t lastKeyNum;
};

#endif
