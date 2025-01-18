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
 * Updated: 29.12.2024
 *  
 * http://loetmeister.de/Elektronik/homematic/index.htm#modules
 */

#ifndef HBWSwitchSerialAdvanced_h
#define HBWSwitchSerialAdvanced_h

#include <HBWired.h>
#include <HBWSwitchAdvanced.h>  // derive from this class
#include "ShiftRegister74HC595.h" // shift register library, slightly customized

// #define DEBUG_OUTPUT   // extra debug output on serial/USB


#define SHIFT_REGISTER_CLASS ShiftRegister74HC595<3>  // Daisy chain 3 registers

#define RELAY_PULSE_DUARTION 80  // HIGH duration in ms, to set or reset double coil latching relay safely

#define OFF false
#define ON true


class HBWSwitchSerialAdvanced : public HBWSwitchAdvanced {
  public:
    HBWSwitchSerialAdvanced(uint8_t _relayPos, uint8_t _ledPos, SHIFT_REGISTER_CLASS* _shiftRegister, hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    // virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data); - use parent
    virtual void afterReadConfig();

  private:
    uint8_t relayPos; // bit position for actual IO port
    uint8_t ledPos;
    SHIFT_REGISTER_CLASS* shiftRegister;  // allow function calls to the correct shift register
    virtual bool setOutput(HBWDevice*, uint8_t newstate);
    
  protected:
    bool relayOperationPending;
    bool relayLevel;  // current logical level
    void operateRelay(bool _newLevel);
    uint32_t relayOperationTimeStart;
};

#endif
