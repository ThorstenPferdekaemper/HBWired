/*
 * HBWSwitchSerialAdvanced.cpp
 * 
 * Ansteuerung von bistabilen Relais über Shiftregister
 * 
 * Als Alternative zu HBWSwitch & HBWLinkSwitchSimple sind mit
 * HBWSwitchAdvanced & HBWLinkSwitchAdvanced folgende Funktionen möglich:
 * Peering mit TOGGLE, TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, onTime,
 * offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung).
 * 
 * Updated: 02.01.2020
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
// shift register library
#include "ShiftRegister74HC595.h"

#define DEBUG_OUTPUT   // extra debug output on serial/USB

#define SHIFT_REGISTER_CLASS ShiftRegister74HC595<3>  // Daisy chain 3 registers

#define RELAY_PULSE_DUARTION 80  // HIG duration in ms, to set or reset double coil latching relay


//// peering/link values must match the XML/EEPROM values!
//#define JT_ONDELAY 0x00
//#define JT_ON 0x01
//#define JT_OFFDELAY 0x02
//#define JT_OFF 0x03
//#define JT_NO_JUMP_IGNORE_COMMAND 0x04
//
//
//struct hbw_config_switch {
//  uint8_t logging:1;              // 0x0000
//  uint8_t output_unlocked:1;      // 0x07:1    0=LOCKED, 1=UNLOCKED
//  uint8_t n_inverted:1;           // 0x07:2    0=inverted, 1=not inverted (device reset will set to 1!)
//  uint8_t        :5;              // 0x0000
//  uint8_t dummy;
//};



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
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending
    
    bool operateRelay;
    uint32_t relayOperationTimeStart;
    
    // set from links/peering (implements state machine)
    void setOutput(HBWDevice*, uint8_t const * const data);
    HBWlibStateMachine StateMachine;
    
};


#endif
