/*
 * HBWSwitchSerialAdvanced.cpp
 * 
 * see "HBWSwitchSerialAdvanced.h" for details
 * 
 * Updated: 02.01.2020
 *  
 * http://loetmeister.de/Elektronik/homematic/index.htm#modules
 * 
 */

#include "HBWSwitchSerialAdvanced.h"


// Class HBWSwitchSerialAdvanced
HBWSwitchSerialAdvanced::HBWSwitchSerialAdvanced(uint8_t _relayPos, uint8_t _ledPos, SHIFT_REGISTER_CLASS* _shiftRegister, hbw_config_switch* _config) {
  relayPos = _relayPos;
  ledPos =_ledPos;
  config = _config;
  shiftRegister = _shiftRegister;
  relayOperationPending = false;
  
  StateMachine.init();
  clearFeedback();
//  StateMachine.setCurrentState(UNKNOWN_STATE); - not needed, as using LED shiftRegister state for init and re-reads
};


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWSwitchSerialAdvanced::afterReadConfig() {

  uint8_t level = shiftRegister->get(ledPos);  // read current state (is zero/LOW on device reset), but keeps state on EEPROM re-read!
  
  //perform intial reset (or set if inverterted) for all relays - bistable relays may have random state!
  if (level) {   // set to 0 or 1
    level = (LOW ^ config->n_inverted);
    StateMachine.setCurrentState(JT_ON);
  }
  else {
    level = (HIGH ^ config->n_inverted);
    StateMachine.setCurrentState(JT_OFF);
  }
// TODO: add zero crossing function?
  operateRelay(level);
  StateMachine.keepCurrentState(); // no action for state machine needed
}


void HBWSwitchSerialAdvanced::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {

  if (length >= 8) {  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
    StateMachine.writePeerParamActionType(*(data));
    uint8_t currentKeyNum = data[7];

#ifdef DEBUG_OUTPUT
  hbwdebug(F("cKeyNum: "));
  hbwdebughex(currentKeyNum);
  hbwdebug(F("\n"));
#endif

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
        StateMachine.keepCurrentState(); // avoid state machine to run
        
#ifdef DEBUG_OUTPUT
  hbwdebug(F("Toggle-> "));
  hbwdebughex(level);
  hbwdebug(F("\n"));
#endif
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
      StateMachine.onDelayTime = data[1];
      StateMachine.onTime = data[2];
      StateMachine.offDelayTime = data[3];
      StateMachine.offTime = data[4];
      StateMachine.jumpTargets.jt_hi_low[0] = data[5];
      StateMachine.jumpTargets.jt_hi_low[1] = data[6];

      StateMachine.forceStateChange(); // force update

#ifdef DEBUG_OUTPUT
  hbwdebug(F("onTime: "));
  hbwdebughex(StateMachine.onTime);
  hbwdebug(F("\n"));
#endif

    }
    StateMachine.lastKeyNum = currentKeyNum;  // store key press number, to identify repeated key events
  }
  else {  // set value - no peering event, overwrite any timer //TODO check: ok to ignore absolute on/off time running? how do original devices handle this?
    //if (!stateTimerRunning)??
    setOutput(device, data);
    StateMachine.stateTimerRunning = false;
    StateMachine.keepCurrentState(); // avoid state machine to run
  }
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWSwitchSerialAdvanced::get(uint8_t* data)
{
  (*data++) = (shiftRegister->get(ledPos)) ? 200 : 0;
  *data = StateMachine.stateTimerRunning ? 64 : 0;  // state flag 'working'
  
  return 2;
};


// set actual outputs
void HBWSwitchSerialAdvanced::setOutput(HBWDevice* device, uint8_t const * const data)
{
  if (config->output_unlocked && relayOperationPending == false) { // not LOCKED and no relay operation pending
    byte level = *(data);

    if (level > 200) // toggle
      level = !shiftRegister->get(ledPos); // get current state and negate
    else if (level) {   // set to 0 or 1
      level = (LOW ^ config->n_inverted);
      shiftRegister->set(ledPos, HIGH); // set LEDs (register used for actual state!)
      StateMachine.setCurrentState(JT_ON);   // update for state machine
    }
    else {
      level = (HIGH ^ config->n_inverted);
      shiftRegister->set(ledPos, LOW); // set LEDs (register used for actual state!)
      StateMachine.setCurrentState(JT_OFF);   // update for state machine
    }
// TODO: add zero crossing function?. Just set portStatus[]? + add portStatusDesired[]?
    operateRelay(level);
  }
  // Logging
  setFeedback(device, config->logging);
};


void HBWSwitchSerialAdvanced::operateRelay(uint8_t _newLevel)
{
//  if (relayOperationPending)
//    return;

  /* if no relay operation is pending, allow so set new state */
  if (_newLevel) { // on - perform set
    shiftRegister->set(relayPos +1, LOW);    // reset coil
    shiftRegister->set(relayPos, HIGH);      // set coil
  }
  else {  // off - perform reset
    shiftRegister->set(relayPos, LOW);      // set coil
    shiftRegister->set(relayPos +1, HIGH);  // reset coil
  }
  
  relayOperationTimeStart = millis();  // relay coil power must be removed after some ms (bistable relays!!)
  relayOperationPending = true;
};


void HBWSwitchSerialAdvanced::loop(HBWDevice* device, uint8_t channel)
{
  unsigned long now = millis();

  /* important to remove power from latching relay after some milliseconds!! */
  if (((now - relayOperationTimeStart) >= RELAY_PULSE_DUARTION) && relayOperationPending == true) {
  // time to remove power from all coils // TODO: implement as timer interrupt routine?
    shiftRegister->setNoUpdate(relayPos +1, LOW);    // reset coil
    shiftRegister->setNoUpdate(relayPos, LOW);       // set coil
    shiftRegister->updateRegisters();
    
    relayOperationPending = false;
  }

  // TODO: move state machine loop to lib HBWlibSwitchAdvanced - shared with HBWSwitchAdvanced //sm_loop(device, channel, currentLevel);?
    
 //*** state machine begin ***//

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
        StateMachine.setLastStateChangeTime(now);
//        StateMachine.setCurrentState(JT_ONDELAY);
        break;
        
      case JT_ON:
        newLevel = 200;
        setNewLevel = true;
        StateMachine.stateTimerRunning = false;
        break;
        
      case JT_OFFDELAY:
        StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offDelayTime);
        StateMachine.setLastStateChangeTime(now);
//        StateMachine.setCurrentState(JT_OFFDELAY);
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
        
      case ON_TIME_MINIMAL:
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
};
