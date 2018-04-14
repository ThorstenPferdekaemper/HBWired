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
    unsigned long stateCangeWaitTime;
    unsigned long lastStateChangeTime;
    uint8_t lastKeyEvent;
};

#endif
