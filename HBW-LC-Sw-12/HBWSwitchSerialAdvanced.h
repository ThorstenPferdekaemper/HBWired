/*
 * HBWSwitchSerialAdvanced.h
 * 
 * Ansteuerung von bistabilen Relais über Shiftregister
 * 
 * Als Alternative zu HBWSwitch & HBWLinkSwitchSimple sind mit
 * HBWSwitchAdvanced & HBWLinkSwitchAdvanced folgende Funktionen möglich:
 * Peering mit TOGGLE, TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, onTime,
 * offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung).
 * 
 * Updated: 01.11.2021
 * 
 * TODO: join redundant code with HBWSwitchAdvanced into one lib
 *  
 * http://loetmeister.de/Elektronik/homematic/index.htm#modules
 */

#ifndef HBWSwitchSerialAdvanced_h
#define HBWSwitchSerialAdvanced_h

#include "HBWlibStateMachine.h"
#include "HBWired.h"
#include "HBWSwitchAdvanced.h"  // take hbw_config_switch and JT_* definition from here
#include "ShiftRegister74HC595.h" // shift register library

#define DEBUG_OUTPUT   // extra debug output on serial/USB

#define SHIFT_REGISTER_CLASS ShiftRegister74HC595<3>  // Daisy chain 3 registers

#define RELAY_PULSE_DUARTION 80  // HIGH duration in ms, to set or reset double coil latching relay safely

#define OFF false
#define ON true


class HBWSwitchSerialAdvanced : public HBWChannel {
  public:
    HBWSwitchSerialAdvanced(uint8_t _relayPos, uint8_t _ledPos, SHIFT_REGISTER_CLASS* _shiftRegister, hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();

  private:
    uint8_t relayPos; // bit position for actual IO port
    uint8_t ledPos;
    SHIFT_REGISTER_CLASS* shiftRegister;  // allow function calls to the correct shift register
    hbw_config_switch* config; // logging
    uint8_t getJumpTarget(uint8_t bitshift) {
      return StateMachine.getJumpTarget(bitshift, JT_ON, JT_OFF);
    };
    
    // set from links/peering (implements state machine)
    HBWlibStateMachine StateMachine;
    
  protected:
    void setOutput(HBWDevice*, uint8_t const * const data);
    bool relayOperationPending;
    bool relayLevel;  // current logical level
    void operateRelay(uint8_t _newLevel);
    uint32_t relayOperationTimeStart;
};


#endif
