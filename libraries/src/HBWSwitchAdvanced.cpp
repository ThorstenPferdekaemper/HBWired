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

#include "HBWSwitchAdvanced.h"


// Switches
HBWSwitchAdvanced::HBWSwitchAdvanced(uint8_t _pin, hbw_config_switch* _config) {
  pin = _pin;
  config = _config;
  nextFeedbackDelay = 0;
  lastFeedbackTime = 0;
  
  StateMachine.onTime = 0xFF;
  StateMachine.offTime = 0xFF;
  StateMachine.jumpTargets.DWORD = 0;
  StateMachine.stateTimerRunning = false;
  StateMachine.stateChangeWaitTime = 0;
  StateMachine.lastStateChangeTime = 0;
  StateMachine.setCurrentState(UNKNOWN_STATE);
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
  
  StateMachine.avoidStateChange(); // no action for state machine needed
}


void HBWSwitchAdvanced::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  
  if (length >= 8) {  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
    
    StateMachine.writePeerParamActionType(*(data));
    uint8_t currentKeyNum = data[7];

    if (StateMachine.peerParam_getActionType() >1) {   // ACTION_TYPE
      if (!StateMachine.stateTimerRunning && StateMachine.lastKeyNum != currentKeyNum) {   // do not interrupt running timer, ignore LONG_MULTIEXECUTE
        byte level;
        if (StateMachine.peerParam_getActionType() == 2)   // TOGGLE_TO_COUNTER
          level = ((currentKeyNum %2 == 0) ? 0 : 200);  //  (switch ON at odd numbers, OFF at even numbers)
        else if (StateMachine.peerParam_getActionType() == 3)   // TOGGLE_INVERSE_TO_COUNTER
          level = ((currentKeyNum %2 == 0) ? 200 : 0);
        else   // TOGGLE
          level = 255;
        
        setOutput(device, &level);
        StateMachine.avoidStateChange(); // avoid state machine to run
      }
    }
    else if (StateMachine.lastKeyNum == currentKeyNum && !StateMachine.peerParam_getLongMultiexecute()) {
      // repeated key event for ACTION_TYPE == 1 (ACTION_TYPE == 0 already filtered by receiveKeyEvent, HBWLinkReceiver)
      // must be long press, but LONG_MULTIEXECUTE not enabled
    }
    else {
      // assign values based on EEPROM layout
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
    setOutput(device, data);
    StateMachine.stateTimerRunning = false;
    StateMachine.avoidStateChange(); // avoid state machine to run
  }
};


uint8_t HBWSwitchAdvanced::get(uint8_t* data) {
  
  if (digitalRead(pin) ^ config->n_inverted)
    (*data) = 0;
  else
    (*data) = 200;
  return 1;
};


void HBWSwitchAdvanced::setOutput(HBWDevice* device, uint8_t const * const data) {
  
  if (config->output_unlocked) {  //0=LOCKED, 1=UNLOCKED
    if(*data > 200) {   // toggle
      digitalWrite(pin, digitalRead(pin) ? LOW : HIGH);
    }
    else {   // on or off
      if (*data) {
        digitalWrite(pin, LOW ^ config->n_inverted);
        StateMachine.setCurrentState(JT_ON);
      }
      else {
        digitalWrite(pin, HIGH ^ config->n_inverted);
        StateMachine.setCurrentState(JT_OFF);
      }
    }
  }
  // Logging
  if(!nextFeedbackDelay && config->logging) {
    lastFeedbackTime = millis();
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
};


void HBWSwitchAdvanced::loop(HBWDevice* device, uint8_t channel) {

  unsigned long now = millis();

 //*** state machine begin ***//

  if (((now - StateMachine.lastStateChangeTime > StateMachine.stateChangeWaitTime) && StateMachine.stateTimerRunning) || StateMachine.getCurrentState() != StateMachine.getNextState()) {

    if (StateMachine.getCurrentState() == StateMachine.getNextState())  // no change to state, so must be time triggered
      StateMachine.stateTimerRunning = false;

#ifndef NO_DEBUG_OUTPUT
  hbwdebug(F("chan:"));
  hbwdebughex(channel);
  hbwdebug(F(" cs:"));
  hbwdebughex(StateMachine.getCurrentState());
#endif
    
    // check next jump from current state
    switch (StateMachine.getCurrentState()) {
      case JT_ONDELAY:      // jump from on delay state
        StateMachine.setNextState(StateMachine.getJumpTarget(0, JT_ON, JT_OFF));
        break;
      case JT_ON:       // jump from on state
        StateMachine.setNextState(StateMachine.getJumpTarget(3, JT_ON, JT_OFF));
        break;
      case JT_OFFDELAY:    // jump from off delay state
        StateMachine.setNextState(StateMachine.getJumpTarget(6, JT_ON, JT_OFF));
        break;
      case JT_OFF:      // jump from off state
        StateMachine.setNextState(StateMachine.getJumpTarget(9, JT_ON, JT_OFF));
        break;
    }
#ifndef NO_DEBUG_OUTPUT
  hbwdebug(F(" ns:"));
  hbwdebughex(StateMachine.getNextState());
  hbwdebug(F("\n"));
#endif

    bool setNewLevel = false;
    uint8_t newLevel = 0;   // default value. Will only be set if setNewLevel was also set 'true'
    uint8_t currentLevel;
    get(&currentLevel);
 
    if (StateMachine.getNextState() < JT_NO_JUMP_IGNORE_COMMAND) {
      
      switch (StateMachine.getNextState()) {
        case JT_ONDELAY:
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.onDelayTime);
          StateMachine.lastStateChangeTime = now;
          StateMachine.stateTimerRunning = true;
          StateMachine.setCurrentState(JT_ONDELAY);
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
          StateMachine.setCurrentState(JT_OFFDELAY);
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
          StateMachine.stateTimerRunning = true;
          StateMachine.setNextState(JT_ON);
          break;
          
        case OFF_TIME_ABSOLUTE:
          //newLevel = 0; // 0 is default
          setNewLevel = true;
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offTime);
          StateMachine.lastStateChangeTime = now;
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
      }
    }
    else {  // NO_JUMP_IGNORE_COMMAND
      StateMachine.setCurrentState(currentLevel ? JT_ON : JT_OFF);    // get current level and update state // TODO: actually needed? or keep for robustness?
      StateMachine.avoidStateChange();   // avoid to run into a loop
    }
    if (currentLevel != newLevel && setNewLevel) {   // check for current level. don't set same level again
      setOutput(device, &newLevel);
    }
  }
  //*** state machine end ***//

  
  // feedback trigger set?
    if(!nextFeedbackDelay) return;
    now = millis();
    if(now - lastFeedbackTime < nextFeedbackDelay) return;
    lastFeedbackTime = now;  // at least last time of trying
    // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
  // we know that the level has only 1 byte here
  uint8_t level;
    get(&level);  
    uint8_t errcode = device->sendInfoMessage(channel, 1, &level);   
    if(errcode == 1) {  // bus busy
    // try again later, but insert a small delay
      nextFeedbackDelay = 250;
    }else{
      nextFeedbackDelay = 0;
    }
}

