/*
 * HBWlibSwitchAdvanced.h
 * 
 * TODO: Update decription
 * 
 * Als Alternative zu HBWSwitch & HBWLinkSwitchSimple sind mit
 * HBWSwitchAdvanced & HBWLinkSwitchAdvanced folgende Funktionen möglich:
 * Peering mit TOGGLE, TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, onTime,
 * offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung).
 * 
 * Updated: 02.06.2020
 * 
 *  
 * http://loetmeister.de/Elektronik/homematic/index.htm#modules
 */

#ifndef HBWlibSwitchAdvanced_h
#define HBWlibSwitchAdvanced_h

#include "HBWlibStateMachine.h"
#include "HBWired.h"


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



class HBWlibSwitchAdvanced : public HBWChannel {
// class HBWlibSwitchAdvanced {
  public:
    // // HBWlibSwitchAdvanced(uint8_t _relayPos, uint8_t _ledPos, SHIFT_REGISTER_CLASS* _shiftRegister, hbw_config_switch* _config);
    // virtual uint8_t get(uint8_t* data);
    // virtual void loop(HBWDevice*, uint8_t channel);
    // virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    // virtual void afterReadConfig();

  private:    

    // enum Actions
    // {
       // INACTIVE = 0,
       // JUMP_TO_TARGET,
       // TOGGLE_TO_COUNTER,
       // TOGGLE_INVERS_TO_COUNTER,
       // TOGGLE
    // };
	
  protected:
    HBWlibStateMachine StateMachine;
    virtual void setOutput(HBWDevice* device, uint8_t const * const data);
	
	uint8_t getJumpTarget(uint8_t bitshift) {
		return StateMachine.getJumpTarget(bitshift, JT_ON, JT_OFF);
	};

inline void sm_loop(HBWDevice* device, uint8_t channel)
// uint8_t sm_loop(HBWDevice* device, uint8_t channel, uint8_t currentLevel);
{
  unsigned long now = millis();

  if (((now - StateMachine.lastStateChangeTime > StateMachine.stateChangeWaitTime) && StateMachine.stateTimerRunning) || !StateMachine.noStateChange()) {

    if (StateMachine.noStateChange())  // no change to state, so must be time triggered
      StateMachine.stateTimerRunning = false;

#ifndef NO_DEBUG_OUTPUT
  hbwdebug(F("chan:"));
  hbwdebughex(channel);
  hbwdebug(F(" state:"));
  hbwdebughex(StateMachine.getCurrentState());
#endif
    
    // check next jump from current state
    switch (StateMachine.getCurrentState()) {
      case JT_ONDELAY:      // jump from on delay state
        // StateMachine.setNextState(StateMachine.getJumpTarget(0, JT_ON, JT_OFF));
        StateMachine.setNextState(getJumpTarget(0));
        break;
      case JT_ON:       // jump from on state
        // StateMachine.setNextState(StateMachine.getJumpTarget(3, JT_ON, JT_OFF));
        StateMachine.setNextState(getJumpTarget(3));
        break;
      case JT_OFFDELAY:    // jump from off delay state
        // StateMachine.setNextState(StateMachine.getJumpTarget(6, JT_ON, JT_OFF));
        StateMachine.setNextState(getJumpTarget(6));
        break;
      case JT_OFF:      // jump from off state
        // StateMachine.setNextState(StateMachine.getJumpTarget(9, JT_ON, JT_OFF));
        StateMachine.setNextState(getJumpTarget(9));
        break;
    }
#ifndef NO_DEBUG_OUTPUT
  hbwdebug(F("->"));
  hbwdebughex(StateMachine.getNextState());
  hbwdebug(F("\n"));
#endif

    StateMachine.absoluteTimeRunning = false;
    bool setNewLevel = false;
    uint8_t newLevel = 0;   // default value. Will only be set if setNewLevel was also set 'true'
    uint8_t currentLevel;
    get(&currentLevel);
 
    switch (StateMachine.getNextState()) {
      case JT_ONDELAY:
        StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.onDelayTime);
        StateMachine.lastStateChangeTime = now;
        StateMachine.stateTimerRunning = true;
        // StateMachine.setCurrentState(JT_ONDELAY);
        break;
        
      case JT_ON:
        newLevel = 200;
        setNewLevel = true;
        StateMachine.stateTimerRunning = false;
        break;
        
      case JT_OFFDELAY:
        StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offDelayTime);
        StateMachine.lastStateChangeTime = now;
        StateMachine.stateTimerRunning = true;
        // StateMachine.setCurrentState(JT_OFFDELAY);
        break;
        
      case JT_OFF:
        //newLevel = 0; // 0 is default
        setNewLevel = true;
        StateMachine.stateTimerRunning = false;
        break;
        
      case ON_TIME_ABSOLUTE:
        newLevel = 200;
        setNewLevel = true;
        StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.onTime);
        StateMachine.lastStateChangeTime = now;
        StateMachine.absoluteTimeRunning = true;
        StateMachine.stateTimerRunning = true;
        StateMachine.setNextState(JT_ON);
        break;
        
      case OFF_TIME_ABSOLUTE:
        //newLevel = 0; // 0 is default
        setNewLevel = true;
        StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offTime);
        StateMachine.lastStateChangeTime = now;
        StateMachine.absoluteTimeRunning = true;
        StateMachine.stateTimerRunning = true;
        StateMachine.setNextState(JT_OFF);
        break;
        
      case ON_TIME_MINIMAL:
        newLevel = 200;
        setNewLevel = true;
        if (now - StateMachine.lastStateChangeTime < StateMachine.convertTime(StateMachine.onTime)) {
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.onTime);
          StateMachine.lastStateChangeTime = now;
          StateMachine.stateTimerRunning = true;
        }
        StateMachine.setNextState(JT_ON);
        break;
        
      case OFF_TIME_MINIMAL:
        //newLevel = 0; // 0 is default
        setNewLevel = true;
        if (now - StateMachine.lastStateChangeTime < StateMachine.convertTime(StateMachine.offTime)) {
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offTime);
          StateMachine.lastStateChangeTime = now;
          StateMachine.stateTimerRunning = true;
        }
        StateMachine.setNextState(JT_OFF);
        break;
      
      default:
        StateMachine.setCurrentState(currentLevel ? JT_ON : JT_OFF );    // get current level and update state, TODO: actually needed? or keep for robustness?
        StateMachine.keepCurrentState();   // avoid to run into a loop
        break;
    }
    StateMachine.setCurrentState(StateMachine.getNextState());  // save new state (currentState = nextState)
    
    if (currentLevel != newLevel && setNewLevel) {   // check for current level. don't set same level again
      setOutput(device, &newLevel);
    }
  }
};


};

#endif
