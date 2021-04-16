/* 
* HBWSwitchAdvanced.cpp
*
* Als Alternative zu HBWSwitch & HBWLinkSwitchSimple sind mit
* HBWSwitchAdvanced & HBWLinkSwitchAdvanced folgende Funktionen möglich:
* Peering mit TOGGLE, TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, onTime,
* offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung).
*
* http://loetmeister.de/Elektronik/homematic/
*
*/

#include "HBWSwitchAdvanced.h"


// Switches
HBWSwitchAdvanced::HBWSwitchAdvanced(uint8_t _pin, hbw_config_switch* _config) {
  pin = _pin;
  config = _config;
  clearFeedback();
  StateMachine.init();
};


// channel specific settings or defaults
void HBWSwitchAdvanced::afterReadConfig() {
  
  if (StateMachine.currentStateIs(UNKNOWN_STATE)) {
  // All off on init, but consider inverted setting
    digitalWrite(pin, config->n_inverted ? LOW : HIGH);    // 0=inverted, 1=not inverted (device reset will set to 1!)
    pinMode(pin, OUTPUT);
    StateMachine.setCurrentState(JT_OFF);
  }
  else {
  // Do not reset outputs on config change (EEPROM re-reads), but update its state
    if (StateMachine.currentStateIs(JT_ON))
      digitalWrite(pin, LOW ^ config->n_inverted);
    else if (StateMachine.currentStateIs(JT_OFF))
      digitalWrite(pin, HIGH ^ config->n_inverted);
  }
  
  StateMachine.keepCurrentState(); // no action for state machine needed
}


void HBWSwitchAdvanced::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  
  if (length == 8) {  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
    
    StateMachine.writePeerParamActionType(*(data));
    uint8_t currentKeyNum = data[7];

    if (StateMachine.peerParam_getActionType() >1) {   // ACTION_TYPE > INACTIVE
      if (!StateMachine.stateTimerRunning && StateMachine.lastKeyNum != currentKeyNum) {   // do not interrupt running timer, ignore LONG_MULTIEXECUTE
        byte level;
        if (StateMachine.peerParam_getActionType() == 2)   // TOGGLE_TO_COUNTER
          level = ((currentKeyNum %2 == 0) ? 0 : 200);  //  (switch ON at odd numbers, OFF at even numbers)
        else if (StateMachine.peerParam_getActionType() == 3)   // TOGGLE_INVERSE_TO_COUNTER
          level = ((currentKeyNum %2 == 0) ? 200 : 0);
        else   // TOGGLE
          level = 255;
        
        setOutput(device, &level);
        StateMachine.keepCurrentState(); // avoid state machine to run
      }
    }
    else if (StateMachine.lastKeyNum == currentKeyNum && !StateMachine.peerParam_getLongMultiexecute()) {
      // repeated key event for ACTION_TYPE == 1 (ACTION_TYPE == 0 already filtered by receiveKeyEvent, HBWLinkReceiver)
      // repeated long press, but LONG_MULTIEXECUTE not enabled
    }
    else if (StateMachine.absoluteTimeRunning && ((StateMachine.currentStateIs(JT_ON) && StateMachine.peerParam_onTimeMinimal()) || (StateMachine.currentStateIs(JT_OFF) && StateMachine.peerParam_offTimeMinimal()))) {
        //do nothing in this case
    }
    else {
      // assign values based on EEPROM layout
	  // TODO: replace with struct? (memcpy(&peerParams, data, NUM_PEER_PARAMS)
	  // FIXME: move ON/OFF_TIME_MINIMAL check here? .. actually this needs full check of JT, etc.? setting the peering parameters here will overwrite all values, like offTime, when it should not (e.g. when new onTime was rejected)
      StateMachine.onDelayTime = data[1];
      StateMachine.onTime = data[2];
      StateMachine.offDelayTime = data[3];
      StateMachine.offTime = data[4];
      StateMachine.jumpTargets.jt_hi_low[0] = data[5];
      StateMachine.jumpTargets.jt_hi_low[1] = data[6];

      StateMachine.forceStateChange(); // force update
    }
    StateMachine.lastKeyNum = currentKeyNum;  // store key press number, to identify repeated key events
  }
  else {  // set value - no peering event, overwrite any timer //TODO check: ok to ignore absolute on/off time running? how do original devices handle this?
    //if (!stateTimerRunning)??
    StateMachine.stateTimerRunning = false;
    setOutput(device, data);
    StateMachine.keepCurrentState(); // avoid state machine to run
  }
};


uint8_t HBWSwitchAdvanced::get(uint8_t* data) {
  
  (*data++) = (digitalRead(pin) ^ config->n_inverted) ? 0 : 200;
  *data = StateMachine.stateTimerRunning ? 64 : 0;  // state flag 'working'
  
  return 2;
};


void HBWSwitchAdvanced::setOutput(HBWDevice* device, uint8_t const * const data) {
  
  if (config->output_unlocked) {  //0=LOCKED, 1=UNLOCKED
  byte level = *(data);
  
    if(level > 200) {   // toggle
	  level = !digitalRead(pin);
    }
    // turn on or off
    if (level) {
      digitalWrite(pin, LOW ^ config->n_inverted);
      StateMachine.setCurrentState(JT_ON);
    }
    else {
      digitalWrite(pin, HIGH ^ config->n_inverted);
      StateMachine.setCurrentState(JT_OFF);
    }
  }
  // Logging
  setFeedback(device, config->logging);
};


void HBWSwitchAdvanced::loop(HBWDevice* device, uint8_t channel) {

  unsigned long now = millis();

 //*** state machine begin ***//
// TODO: move state machine loop to lib HBWlibSwitchAdvanced - shared with HBWSwitchAdvanced //sm_loop(device, channel);?

  if (((now - StateMachine.lastStateChangeTime > StateMachine.stateChangeWaitTime) && StateMachine.stateTimerRunning) || !StateMachine.noStateChange()) {

    if (StateMachine.noStateChange())  // no change to state, so must be time triggered
      StateMachine.stateTimerRunning = false;

#ifdef DEBUG_OUTPUT
  hbwdebug(F("chan:"));
  hbwdebughex(channel);
  hbwdebug(F(" state:"));
  hbwdebughex(StateMachine.getCurrentState());
#endif
    
    // check next jump from current state
    switch (StateMachine.getCurrentState()) {
      case JT_ONDELAY:      // jump from on delay state
        StateMachine.setNextState(getJumpTarget(0));
        break;
      case JT_ON:       // jump from on state
        StateMachine.setNextState(getJumpTarget(3));
        break;
      case JT_OFFDELAY:    // jump from off delay state
        StateMachine.setNextState(getJumpTarget(6));
        break;
      case JT_OFF:      // jump from off state
        StateMachine.setNextState(getJumpTarget(9));
        break;
    }
#ifdef DEBUG_OUTPUT
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
        StateMachine.setLastStateChangeTime(now);;
        // StateMachine.setCurrentState(JT_ONDELAY);
        break;
        
      case JT_ON:
        newLevel = 200;
        setNewLevel = true;
        StateMachine.stateTimerRunning = false;
        break;
        
      case JT_OFFDELAY:
        StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offDelayTime);
        StateMachine.setLastStateChangeTime(now);;
        // StateMachine.setCurrentState(JT_OFFDELAY);
        break;
        
      case JT_OFF:
        //newLevel = 0; // 0 is default
        setNewLevel = true;
        StateMachine.stateTimerRunning = false;
        break;
        
      case ON_TIME_ABSOLUTE:  // ABSOLUTE time will always be applied
        newLevel = 200;
        setNewLevel = true;
        StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.onTime);
        StateMachine.setLastStateChangeTime(now);
        StateMachine.absoluteTimeRunning = true;
        StateMachine.setNextState(JT_ON);
        break;
        
      case OFF_TIME_ABSOLUTE:
        //newLevel = 0; // 0 is default
        setNewLevel = true;
        StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offTime);
        StateMachine.setLastStateChangeTime(now);
        StateMachine.absoluteTimeRunning = true;
        StateMachine.setNextState(JT_OFF);
        break;
        
      case ON_TIME_MINIMAL:  // new MINIMAL time will only be applied if current remaining time is shorter
        if ((StateMachine.stateChangeWaitTime - (now - StateMachine.lastStateChangeTime)) < StateMachine.convertTime(StateMachine.onTime) || StateMachine.stateTimerRunning == false ) {
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.onTime);
          StateMachine.setLastStateChangeTime(now);
          newLevel = 200;
          setNewLevel = true;
        }
        StateMachine.setNextState(JT_ON);
        break;
        
      case OFF_TIME_MINIMAL:
        //newLevel = 0; // 0 is default
        if ((StateMachine.stateChangeWaitTime - (now - StateMachine.lastStateChangeTime)) < StateMachine.convertTime(StateMachine.offTime) || StateMachine.stateTimerRunning == false ) {
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offTime);
          StateMachine.setLastStateChangeTime(now);
          setNewLevel = true;
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
  //*** state machine end ***//
  
  // feedback trigger set?
  checkFeedback(device, channel);
}

