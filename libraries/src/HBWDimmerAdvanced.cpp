/* 
* HBWDimmerAdvanced
*
* Mit HBWDimmerAdvanced & HBWLinkDimmerAdvanced sind folgende Funktionen möglich:
* Peering mit TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, UPDIM, DOWNDIM,
* TOGGLEDIM, TOGGLEDIM_TO_COUNTER, TOGGLEDIM_INVERSE_TO_COUNTER, onTime,
* offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung)
* RampOn, RampOff.
*
* http://loetmeister.de/Elektronik/homematic/
*
* Last updated: 12.01.2025
*/

#include "HBWDimmerAdvanced.h"

// Dimming channel
HBWDimmerAdvanced::HBWDimmerAdvanced(uint8_t _pin, hbw_config_dim* _config)
{
  pin = _pin;
  config = _config;
  currentLevel = 0;
  oldOnLevel = 0;
  destLevel = currentLevel;
  dimmingDirectionUp = false;
  clearFeedback();
  init();
};


// channel specific settings or defaults
void HBWDimmerAdvanced::afterReadConfig()
{
  if (currentState == UNKNOWN_STATE) {
    // All off on init
    analogWrite(pin, LOW);
    currentState = JT_OFF;
  }
  else {
  // Do not reset outputs on config change, but update its state (to apply new settings from EEPROM)
    setOutputNoLogging(currentLevel);
  }
};


/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWDimmerAdvanced::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == NUM_PEER_PARAMS +2)  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
  {
    s_peering_list* peeringList;
    peeringList = (s_peering_list*)data;
    uint8_t currentKeyNum = data[NUM_PEER_PARAMS];
    bool sameLastSender = data[NUM_PEER_PARAMS +1];

    if ((lastKeyNum == currentKeyNum && sameLastSender) && !peeringList->LongMultiexecute)
      return;  // repeated long press reveived, but LONG_MULTIEXECUTE not enabled
    // TODO: check behaviour for LONG_MULTIEXECUTE & TOGGLE... (long toggle?)

	hbwdebug(F("\nonDelayTime "));hbwdebug(peeringList->onDelayTime);hbwdebug(F("\n"));
	hbwdebug(F("onTime "));hbwdebug(peeringList->onTime);hbwdebug(F("\n"));
	hbwdebug(F("onTimeMin "));hbwdebug(peeringList->isOnTimeMinimal());hbwdebug(F("\n"));
	hbwdebug(F("offDelayTime "));hbwdebug(peeringList->offDelayTime);hbwdebug(F("\n"));
	hbwdebug(F("offDelayBlink "));hbwdebug(peeringList->offDelayBlink);hbwdebug(F("\n"));
	hbwdebug(F("offDelayStep "));hbwdebug(peeringList->offDelayStep*2);hbwdebug(F("\n"));
	hbwdebug(F("offTime "));hbwdebug(peeringList->offTime);hbwdebug(F("\n"));
	hbwdebug(F("offTimeMin "));hbwdebug(peeringList->isOffTimeMinimal());hbwdebug(F("\n"));
	// hbwdebug(F("jtOnDelay "));hbwdebug(peeringList->jtOnDelay);hbwdebug(F("\n"));
	// hbwdebug(F("jtRampOn "));hbwdebug(peeringList->jtRampOn);hbwdebug(F("\n"));
	// hbwdebug(F("jtOn "));hbwdebug(peeringList->jtOn);hbwdebug(F("\n"));
	// hbwdebug(F("jtOffDelay "));hbwdebug(peeringList->jtOffDelay);hbwdebug(F("\n"));
	// hbwdebug(F("jtRampOff "));hbwdebug(peeringList->jtRampOff);hbwdebug(F("\n"));
	// hbwdebug(F("jtOff "));hbwdebug(peeringList->jtOff);hbwdebug(F("\n"));
	hbwdebug(F("offLevel "));hbwdebug(peeringList->offLevel);hbwdebug(F("\n"));
	hbwdebug(F("onLevel "));hbwdebug(peeringList->onLevel);hbwdebug(F("\n"));
	hbwdebug(F("onLevelPrio "));hbwdebug(peeringList->onLevelPrio);hbwdebug(F("\n"));
	hbwdebug(F("onMinLevel "));hbwdebug(peeringList->onMinLevel);hbwdebug(F("\n"));
	hbwdebug(F("rStartStep "));hbwdebug(peeringList->rampStartStep);hbwdebug(F("\n"));

    if (peeringList->actionType == JUMP_TO_TARGET)
    {
      hbwdebug(F("jumpToTarget\n"));
      // s_jt_peering_list* jtPeeringList;
      // jtPeeringList = (s_jt_peering_list*)&data[5];
      jumpToTarget(device, peeringList);//, jtPeeringList);
    }
    else if (peeringList->actionType >= TOGGLE_TO_COUNTER && peeringList->actionType <= TOGGLEDIM_INVERS_TO_COUNTER)
    {
      uint8_t nextState;
      uint8_t newlevel = currentLevel;
     hbwdebug(F("kNum:"));hbwdebug(currentKeyNum);hbwdebug(F(" "));
      switch (peeringList->actionType) {
      case TOGGLE_TO_COUNTER:
         hbwdebug(F("TOGGLE_TO_C\n"));  // switch ON at odd numbers, OFF at even numbers
         // newlevel = (currentKeyNum & 1) ? peeringList->onLevel : peeringList->offLevel;
         nextState = (currentKeyNum & 0x01) == 0x01 ? JT_RAMPON : JT_RAMPOFF;
      // uint32_t delay = getDelayForState(nextState, peeringList);  // get on/off time also for toggle actions
      // setState(device, nextState, delay, peeringList);
         break;
       case TOGGLE_INVERS_TO_COUNTER:
        hbwdebug(F("TOGGLE_INV_TO_C\n"));  // switch OFF at odd numbers, ON at even numbers
        // newlevel = (currentKeyNum & 1) ? data[D_POS_offLevel] : data[D_POS_onLevel];  // ON/OFF inverse
        nextState = (currentKeyNum & 0x01) == 0x00 ? JT_RAMPON : JT_RAMPOFF;
        break;
      case UPDIM:
        newlevel = dimUpDown(peeringList, DIM_UP);
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;  // TODO: move this to dimUpDown() for all dim actions
        break;
      case DOWNDIM:
        newlevel = dimUpDown(peeringList, DIM_DOWN);  // down dim sets level OFF, if below onMinLevel? onLevel?
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;
        break;
      case TOGGLEDIM:
        newlevel = (dimmingDirectionUp ^ (lastKeyNum == currentKeyNum && sameLastSender)) ? dimUpDown(peeringList, DIM_DOWN) : dimUpDown(peeringList, DIM_UP);
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;
        break;
      case TOGGLEDIM_TO_COUNTER:
        newlevel = (currentKeyNum & 1) ? dimUpDown(peeringList, DIM_UP) : dimUpDown(peeringList, DIM_DOWN);  // dim up at odd numbers, dim down at even numbers
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;
        break;
      case TOGGLEDIM_INVERS_TO_COUNTER:
        newlevel = (currentKeyNum & 1) ? dimUpDown(peeringList, DIM_DOWN) : dimUpDown(peeringList, DIM_UP);  // up/down inverse
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;
        break;
      default: break;
      }
      destLevel = newlevel;
      uint32_t delay = getDelayForState(nextState, peeringList);  // get on/off time also for toggle actions
      setState(device, nextState, delay, peeringList);  // dimUpDown will change state, so setState will always set newlevel
      // destLevel = newlevel;  // overwrite calculated level from setState
      // setOutput(device, newlevel);  // nothing happens if old and new level are the same
    }
    lastKeyNum = currentKeyNum;  // store key press number, to identify repeated key events
  }
  else {  // set value - no peering event, overwrite any timer
    //TODO check: how to handle onLevelPrio = HIGH
    // clear stored peeringList on manual SET()
    memset(stateParamList, 0, sizeof(*stateParamList));
    
    uint8_t newState;
    if (*(data) > ON_OLD_LEVEL) {   // use toggle for any other value (202 and greater)
      // // setState(device, ((currentState == JT_OFF || currentState == JT_ONDELAY || currentState == JT_RAMPON) ? JT_ON : JT_OFF), DELAY_INFINITE);
      // // newState = (currentState == JT_OFF || currentState == JT_ONDELAY || currentState == JT_RAMPON) ? JT_ON : JT_OFF;
      // // toggle via ON/OFF RAMP
      // newState = (currentState == JT_OFF || currentState == JT_ONDELAY || currentState == JT_RAMPOFF) ? JT_RAMPON : JT_RAMPOFF;
      setState(device, ((currentState == JT_OFF || currentState == JT_ONDELAY || currentState == JT_RAMPOFF) ? JT_RAMPON : JT_RAMPOFF),  DELAY_INFINITE);
    }
    else {
      if (*(data) == ON_OLD_LEVEL) {   // on with last on-value      
        destLevel = oldOnLevel;
        newState = (destLevel == 0 ? JT_OFF : JT_ON);
        // setState(device, (destLevel == 0 ? JT_OFF : JT_ON), DELAY_INFINITE);
        // setOutput(device, destLevel);  // nothing happens if old and new level are the same
      }
      else {
        // setState(device, *(data) == 0 ? JT_OFF : JT_ON, DELAY_INFINITE);  // set on or off
        destLevel = *(data);
        newState = (destLevel == 0 ? JT_OFF : JT_ON);  // set on or off
      // setState(device, (destLevel == 0 ? JT_OFF : JT_ON), DELAY_INFINITE);  // set on or off
      // setOutput(device, destLevel);  // nothing happens if old and new level are the same
      }
      setState(device, newState, DELAY_INFINITE);  // will not set same state again, so below setOutput() will handle new level in same state
      setOutput(device, destLevel);  // nothing happens if old and new level are the same
    }
    stateParamList->onLevelPrio = ON_LEVEL_PRIO_HIGH;
    // manual set ON/OFF will be without timer (i.e. DELAY_INFINITE) - which only be changed by absolute time peering
    // is onLevelPrio HIGH actually needed? It has no effect on OFF state change, but new on values are only accepted with high prio and absolute time
  }
};


/* private function - returns the new level (0...200) to be set */
uint8_t HBWDimmerAdvanced::dimUpDown(s_peering_list* _peeringList, bool dimUp)
{
  uint8_t level = currentLevel;  // keep current level if we cannot dim up or down anymore
  // limits: dimStep 15% (level 30) + currentLevel 200 max
  
  // UPDIM
  if (dimUp) {
	  hbwdebug(F("dimUp>"));
    level = currentLevel + (_peeringList->dimStep *2);
    // if (currentLevel < _peeringList->dimMaxLevel) { // TODO check for (currentLevel + (peerParam_getDimStep() *2) > dimMaxLevel) directly...
    if (level < _peeringList->dimMaxLevel) { // TODO check for (currentLevel + (peerParam_getDimStep() *2) > dimMaxLevel) directly...
      // level = currentLevel + (_peeringList->dimStep *2);
      // if (level >= _peeringList->dimMaxLevel)
        // level = _peeringList->dimMaxLevel;
      if (level > _peeringList->offLevel && level < _peeringList->onMinLevel) {
        level = _peeringList->onMinLevel;
      }
    }
    else {
      level = _peeringList->dimMaxLevel;
    }
	// currentState = JT_RAMPON;
  }
  // DOWNDIM
  else {
	  hbwdebug(F("dimDown<"));
    level = currentLevel - ((_peeringList->dimStep *2) < currentLevel ? (_peeringList->dimStep *2) : currentLevel);
	// currentState = JT_RAMPON;
    // if (currentLevel >= _peeringList->dimMinLevel) {
    if (level < _peeringList->dimMinLevel) {
		level = _peeringList->dimMinLevel;
	}
      // level = currentLevel - (_peeringList->dimStep *2);
      if (level < _peeringList->onMinLevel) {
        level = _peeringList->offLevel;
		// currentState = JT_RAMPOFF;
	  }
      // if (level < _peeringList->onMinLevel)
        // level = _peeringList->offLevel;  // state = OFF
    // }
    // else
      // level = (level < _peeringList->dimMinLevel) ?_peeringList->dimMinLevel : _peeringList->offLevel; //data[D_POS_dimMinLevel];// switchState(JT_ON
  // TODO set state? - no... do it outside....
  }
  currentState = TEMP_DIMMING_STATE;  // avoid checking agaist ON state, that would change the destLevel
  hbwdebug(level);
  hbwdebug(F("\n"));
  return level;
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDimmerAdvanced::get(uint8_t* data)
{
  state_flags stateFlags;
  stateFlags.byte = 0;
  
  //TODO: show direction also in UP/Down dim?
  if (currentState == JT_RAMPON || currentState == JT_RAMPOFF) {
    stateFlags.state.upDown = dimmingDirectionUp ? STATEFLAG_DIM_UP : STATEFLAG_DIM_DOWN;
  }
  else {
    // state up or down also sets channel "working" - don't set both at the same time
    stateFlags.state.working = stateTimerRunning ? true : false;
  }
  
  *data++ = currentLevel;
  *data = stateFlags.byte;
  
  return 2;
};


/* private function - sets/operates the actual outputs. Sets timer for logging/i-message */
void HBWDimmerAdvanced::setOutput(HBWDevice* device, uint8_t _newLevel)
{
#ifdef DEBUG_OUTPUT
  hbwdebug(F("l-"));    // indicate set output _with_ logging
#endif
  
  // if (_newLevel == ON_OLD_LEVEL) {  // use OLD_LEVEL for ON_LEVEL
    // // if (oldOnLevel > stateParamList->onMinLevel)
      // _newLevel = oldOnLevel;
    // // else
      // // _newLevel = stateParamList->onMinLevel; // TODO: what to use? onMinLevel or offLevel? can old-level be off?
  // }
  
  if (currentLevel != _newLevel) {  // only set output and send i-message if value was changed
    setOutputNoLogging(_newLevel);
    // Logging
    setFeedback(device, config->logging);
  }
#ifdef DEBUG_OUTPUT
  else {
  hbwdebug(F("no change"));
  hbwdebug(F("\n"));
  }
#endif
};


/* private function - sets/operates the actual outputs. No logging/i-message! */
// keep code small, as this function might be called quickly again
bool HBWDimmerAdvanced::setOutputNoLogging(uint8_t _newLevel)
{
  if (config->pwm_range == 0) {  // 0=disabled
    analogWrite(pin, LOW);
    currentState = JT_OFF;
    return false;
  }
  if (_newLevel > 200)  _newLevel = 200;  // don't exceed limit
  
  //                              scale to 40%   50%   60%   70%   80%   90%  100% - according to pwm_range setting
  static const uint16_t newValueMax[7] = {1020, 1275, 1530, 1785, 2040, 2300, 2550};  // avoid float, divide by 10 when calling analogWrite()!
  uint8_t newValueMin = 0;

  if (!config->voltage_default) newValueMin = 255; // Factor 10! Set 25.5 min output level (needs 0.5-5V for 1-10V mode)
  
  if (_newLevel > currentLevel)  dimmingDirectionUp = true;
  else                          dimmingDirectionUp = false;
  
  currentLevel = _newLevel;

  // set value
  uint8_t newPwmValue = (map(_newLevel, 0, 200, newValueMin, newValueMax[config->pwm_range -1])) /10;  // map 0-200 into correct PWM range
  analogWrite(pin, newPwmValue);
  
#ifdef DEBUG_OUTPUT
  hbwdebug(F(" (lvl:"));
  hbwdebug(currentLevel);
  hbwdebug(F(" up:"));
  hbwdebug(dimmingDirectionUp);
  hbwdebug(F(" PWM:"));
  hbwdebug(newPwmValue);
  hbwdebug(F(" max:"));
  hbwdebug((newValueMax[config->pwm_range -1])/10);
  hbwdebug(F(") "));
#endif
  return true;
};


/* private function - sets all variables required for On/OffRamp */
/* void HBWDimmerAdvanced::prepareOnOffRamp(uint8_t rampTime, uint8_t level)
{
// int rampStep = (targetLevel - curentLevel) / rampTime;  if (rampStep < 0) {directionUp = true; rampStep *=-1; else directionUp = false; }
// if (rampStep < 0) {dimUp} else {dimDown}
  if (rampTime != 255 && rampTime != 0 && (level > StateMachine.onMinLevel)) {   // time == 0xFF when not used  
    StateMachine.changeWaitTime = StateMachine.convertTime(rampTime);
    offDlyBlinkCounter = (level - StateMachine.onMinLevel) *10;  // do not create overflow by subtraction (level > onMinLevel)
    
    if (StateMachine.changeWaitTime > offDlyBlinkCounter * (RAMP_MIN_STEP_WIDTH/10)) {
      rampStep = 10;      // factor 10 (1 step = 0.5%)
      StateMachine.changeWaitTime = StateMachine.changeWaitTime / (offDlyBlinkCounter /10);
    }
    else {
      rampStep = offDlyBlinkCounter / (StateMachine.changeWaitTime / RAMP_MIN_STEP_WIDTH);  // factor 10
      StateMachine.changeWaitTime = RAMP_MIN_STEP_WIDTH;
    }
  }
  else {
    offDlyBlinkCounter = 0;
    rampStep = 0;
    StateMachine.changeWaitTime = 0;
  }
  
  StateMachine.setLastStateChangeTime(millis());

#ifdef DEBUG_OUTPUT
  hbwdebug(F("stChgWaitTime: "));
  hbwdebug(StateMachine.changeWaitTime);
  hbwdebug(F(" RmpStCt: "));
  hbwdebug(offDlyBlinkCounter);
  hbwdebug(F(" RmpSt: "));
  hbwdebug(rampStep);
  hbwdebug(F("\n"));
#endif
};*/


/* bool HBWDimmerAdvanced::checkOnLevelPrio(void)
{
 #ifdef DEBUG_OUTPUT
   hbwdebug(F("onLvlPrio:"));
   hbwdebug(currentOnLevelPrio == ON_LEVEL_PRIO_HIGH);
 #endif

  if (currentOnLevelPrio == StateMachine.peerParam_getOnLevelPrio() || StateMachine.peerParam_onLevelPrioIsHigh()) {
    currentOnLevelPrio = StateMachine.peerParam_getOnLevelPrio();

 #ifdef DEBUG_OUTPUT
  hbwdebug(F(" newPrio:"));
  hbwdebug(currentOnLevelPrio == ON_LEVEL_PRIO_HIGH);
  hbwdebug(F("\n"));
 #endif

    return true;
  }
  else {

 #ifdef DEBUG_OUTPUT
  hbwdebug(F("\n"));
 #endif

    return false;
  }
}*/


/* standard public function - called by main loop for every channel in sequential order */
void HBWDimmerAdvanced::loop(HBWDevice* device, uint8_t channel)
{
  if (setDestLevelOnce) {
    setDestLevelOnce = false;
    setOutput(device, destLevel);
	// TODO if blink should be done here... (setDestLevelOnce == true && offDlyBlinkCounter)... let this run in parallel with regular offDelayTime timer
  }
  
  unsigned long now = millis();

 //*** state machine begin ***//

  if ((now - lastChangeTime > changeWaitTime) && stateTimerRunning) // TODO : rename to lastChangeTime & changeWaitTime
  {
    stopStateChangeTime();  // time was up, so don't come back into here - new valid timer should be set below - or resumed
    uint8_t newLevel = currentLevel;
	
   // #ifdef DEBUG_OUTPUT
     // hbwdebug(F("chan:"));
     // hbwdebughex(channel);
   // #endif
   
    // JT_OFFDELAY blink - calculate new time & level. Level is set by above up/down "loop"
    // rampStep is level change (offDelayStep); offDlyBlinkCounter is the number of blink times
    if (offDlyBlinkCounter) {// && currentState == JT_OFFDELAY) {
      if (dimmingDirectionUp == false) {
        // newLevel = currentLevel - rampStep;
        destLevel = currentLevel + rampStep;
        changeWaitTime = stateParamList->offDelayOldTime *1000;
        if (offDlyBlinkCounter > 0) { offDlyBlinkCounter--; }  // end at initial level
		// else last step... get remaining time?
		// else changeWaitTime = convertTime(stateParamList->offDelayTime) - ((uint32_t)(stateParamList->offDelayNewTime + stateParamList->offDelayOldTime) *1000) *((convertTime(stateParamList->offDelayTime) /1000) / (stateParamList->offDelayNewTime + stateParamList->offDelayOldTime));
      }
      else {
        // newLevel = currentLevel + rampStep;
        destLevel = currentLevel - rampStep;
        changeWaitTime = stateParamList->offDelayNewTime *1000;
      }
    }
   
    if (currentLevel != destLevel) {
// hbwdebug(F(" loop u/d "));
      uint8_t diff;
      if (currentLevel > destLevel ) { // dim down
        diff = currentLevel - destLevel;
        // updateLevel(currentLevel - (diff < rampStep ? diff : rampStep));
        newLevel = (currentLevel - (diff < rampStep ? diff : rampStep));
      }
      else { // dim up
        diff = destLevel - currentLevel;
        // updateLevel(currentLevel + (diff < rampStep ? diff : rampStep));
        newLevel = (currentLevel + (diff < rampStep ? diff : rampStep));
      }
	  // hbwdebug(F("nlvl:"));hbwdebug(newLevel);hbwdebug(F(" dst:"));hbwdebug(destLevel);hbwdebug(F(" diff:"));hbwdebug(diff);
	  	// setOutputNoLogging(newLevel);
    }
	
    // we catch our destination level -> go to next state
    if (currentLevel == destLevel) { // TOOD: only check destLevel if in RAMP ON / OFF state??
// hbwdebug(F(" loop next "));
      uint8_t nextState = getNextState();
      // if (changeWaitTime == DELAY_NO) {// && stateParamList->actionType != 0) { //stateParamList != NULL ) {
        // changeWaitTime = getDelayForState(nextState, stateParamList);
        uint32_t delay = getDelayForState(nextState, stateParamList);
      // }
      // setState(device, nextState, changeWaitTime, stateParamList);
      setState(device, nextState, delay, stateParamList);
    }
    else { // enable again for next ramp step
	if (setOutputNoLogging(newLevel))
      resetNewStateChangeTime(now);
// hbwdebug(F(" restart! "));
    }
	
	// hbwdebug(F("\n"));
  }



 //*** state machine begin ***//
  /*
  if (((now - StateMachine.lastChangeTime > StateMachine.changeWaitTime) && StateMachine.stateTimerRunning) || !StateMachine.noStateChange()) {
    
    if (StateMachine.getNextState() == FORCE_STATE_CHANGE) {
      offDlyBlinkCounter = 0;  // clear counter, when state change was forced
    }
    
    // on / off ramp
    if (offDlyBlinkCounter && (StateMachine.currentStateIs(JT_RAMPON) || StateMachine.currentStateIs(JT_RAMPOFF)))
    {
      uint8_t rampNewValue;
      StateMachine.lastChangeTime = now;
      
      if (StateMachine.currentStateIs(JT_RAMPON))
        rampNewValue = (StateMachine.onLevel *10 - offDlyBlinkCounter) /10;
      else
        rampNewValue = (offDlyBlinkCounter /10) + StateMachine.onMinLevel;
      
      setOutputNoLogging(rampNewValue);  // never send i-message here. State change to on or off will do it
  
      if (rampStep > offDlyBlinkCounter) offDlyBlinkCounter = 0;
      else  offDlyBlinkCounter -= rampStep;

#ifdef DEBUG_OUTPUT
 hbwdebug(F("RAMP-ct: "));
 hbwdebug(offDlyBlinkCounter);
 hbwdebug(F("\n"));
#endif
    }
    // off delay blink
    else if (offDlyBlinkCounter && StateMachine.currentStateIs(JT_OFFDELAY)) {
      uint8_t newValue;
      
      if (offDelayNewTimeActive) {
        newValue = currentLevel - (peerParam_getOffDelayStep());
        StateMachine.changeWaitTime = peerParam_getOffDelayOldTime() *1000;
        offDelayNewTimeActive = false;
      }
      else {
        newValue = currentLevel + (peerParam_getOffDelayStep());
        StateMachine.changeWaitTime = peerParam_getOffDelayNewTime() *1000;
        offDelayNewTimeActive = true;
        offDlyBlinkCounter--;
      }
      setOutputNoLogging(newValue);
      
      if (offDelaySingleStep) {   // no blink, reduce level only once, reset counter and set wait time
        offDelaySingleStep = false;
        offDlyBlinkCounter = 0;
        StateMachine.changeWaitTime = StateMachine.convertTime(StateMachine.offDelayTime);
      }
      else
        StateMachine.lastChangeTime = now;    // only update in blink mode
    }
    else {
      if (StateMachine.noStateChange()) {  // no change to state, so must be time triggered
        StateMachine.stateTimerRunning = false;
		//TODO: need to clear StateMachine.changeWaitTime and StateMachine.lastChangeTime?
		// replace stateTimerRunning? Use changeWaitTime == 0, as stateTimerRunning = false?
      }

  #ifdef DEBUG_OUTPUT
    hbwdebug(F("chan:"));
    hbwdebughex(channel);
    hbwdebug(F(" state: "));
    hbwdebughex(StateMachine.getCurrentState());
  #endif
      
      // check next jump from current state
      switch (StateMachine.getCurrentState()) {
        case JT_ONDELAY:      // jump from on delay state
          StateMachine.setNextState(getJumpTarget(0));
          break;
        case JT_RAMPON:       // jump from ramp on state
          StateMachine.setNextState(getJumpTarget(12));
          break;
        case JT_ON:       // jump from on state
          oldOnLevel = currentLevel;  // save current on value before off ramp or switching off
          currentOnLevelPrio = ON_LEVEL_PRIO_LOW;  // clear OnLevelPrio when leaving ON state
          StateMachine.setNextState(getJumpTarget(3));
          break;
        case JT_OFFDELAY:    // jump from off delay state
          StateMachine.setNextState(getJumpTarget(6));
          break;
        case JT_RAMPOFF:       // jump from ramp off state
          StateMachine.setNextState(getJumpTarget(15));
          break;
        case JT_OFF:      // jump from off state
          StateMachine.setNextState(getJumpTarget(9));
          break;
      }
  #ifdef DEBUG_OUTPUT
    hbwdebug(F(" -> "));
    hbwdebughex(StateMachine.getNextState());
    hbwdebug(F("\n"));
  #endif
  
      StateMachine.absoluteTimeRunning = false;
      bool setNewLevel = false;
      uint8_t newLevel = StateMachine.offLevel;   // default value. Will only be set if setNewLevel was also set 'true'
      
      switch (StateMachine.getNextState()) {
        case JT_ONDELAY:
          if (StateMachine.peerParam_onDelayModeSetToOff())  setNewLevel = true;  //0=SET_TO_OFF, 1=NO_CHANGE
          StateMachine.changeWaitTime = StateMachine.convertTime(StateMachine.onDelayTime);
          StateMachine.setLastStateChangeTime(now);
          break;

        case JT_RAMPON:
#ifdef DEBUG_OUTPUT
  hbwdebug(F("onLvl: "));
  hbwdebug(StateMachine.onLevel);
#endif
          if (StateMachine.onLevel == ON_OLD_LEVEL) {  // use OLD_LEVEL for ON_LEVEL
            if (oldOnLevel > StateMachine.onMinLevel) {
              StateMachine.onLevel = oldOnLevel;
            }
            else {
              StateMachine.onLevel = StateMachine.onMinLevel; // TODO: what to use? onMinLevel or offLevel? or RAMP_START_STEP?
              rampOnTime = 0;   // no ramp to set onMinLevel
            }
          }
#ifdef DEBUG_OUTPUT
  hbwdebug(F(" oldVal: "));
  hbwdebug(oldOnLevel);
  hbwdebug(F(" new onLvl: "));
  hbwdebug(StateMachine.onLevel);
  hbwdebug(F("\n"));
#endif
          prepareOnOffRamp(rampOnTime, StateMachine.onLevel);
          offDlyBlinkCounter -= rampStep; // reduce by one step, as we go to min. on level immediately
          newLevel = StateMachine.onMinLevel;
          setNewLevel = true;
          break;
        
        case JT_ON:
          if (checkOnLevelPrio()) {
            newLevel = StateMachine.onLevel;
            setNewLevel = true;
          }
          StateMachine.stateTimerRunning = false;
          break;
          
        case JT_OFFDELAY:
          StateMachine.changeWaitTime = StateMachine.convertTime(StateMachine.offDelayTime);
          // only reduce level, if not going below min. on level
      // TODO: check if should be forced to blink, by: onMinLevel + offDelayStep ??
#ifdef DEBUG_OUTPUT
  hbwdebug(F("OffDlySt:"));
  hbwdebug(peerParam_getOffDelayStep());
#endif
          if (peerParam_getOffDelayStep() && (currentLevel - StateMachine.onMinLevel) > (peerParam_getOffDelayStep())) {
            offDelayNewTimeActive = true;
            StateMachine.changeWaitTime = 0; // start immediately
            
            if (StateMachine.peerParam_offDelayBlinkEnabled()) {  // calculate total level changes for off delay blink duration
              offDlyBlinkCounter = ((StateMachine.convertTime(StateMachine.offDelayTime) /1000) / (peerParam_getOffDelayNewTime() + peerParam_getOffDelayOldTime()));
#ifdef DEBUG_OUTPUT
  hbwdebug(F(" blink:"));
#endif
            }
            else {  // only reduce level, no blink
              offDlyBlinkCounter = 1;
              offDelaySingleStep = true;
#ifdef DEBUG_OUTPUT
  hbwdebug(F(" nBlink:"));
#endif
            }
          }
          StateMachine.setLastStateChangeTime(now);
#ifdef DEBUG_OUTPUT
  hbwdebug(offDlyBlinkCounter);
  hbwdebug(F("\n"));
#endif
          break;
          
        case JT_RAMPOFF:
          prepareOnOffRamp(rampOffTime, currentLevel);
          break;
          
        case JT_OFF:
          setNewLevel = true; // offLevel is default, no need to set newLevel
          StateMachine.stateTimerRunning = false;
          break;
          
        case ON_TIME_ABSOLUTE:  // ABSOLUTE time will always be applied (unless new on prio is lower than current prio)
          checkOnLevelPrio();
          newLevel = StateMachine.onLevel;
          setNewLevel = true;
          StateMachine.changeWaitTime = StateMachine.convertTime(StateMachine.onTime);
          StateMachine.setLastStateChangeTime(now);
          StateMachine.absoluteTimeRunning = true;
          StateMachine.setNextState(JT_ON);
          break;
          
        case OFF_TIME_ABSOLUTE:
          setNewLevel = true; // offLevel is default, no need to set newLevel
          StateMachine.changeWaitTime = StateMachine.convertTime(StateMachine.offTime);
          StateMachine.setLastStateChangeTime(now);
          StateMachine.absoluteTimeRunning = true;
          StateMachine.setNextState(JT_OFF);
          break;
          
        case ON_TIME_MINIMAL:  // new MINIMAL time will only be applied if current remaining time is shorter
         // TODO: validation against remaining time (lastChangeTime) ist ok? or should be old onTime (changeWaitTime)?
          if ((StateMachine.changeWaitTime - (now - StateMachine.lastChangeTime)) < StateMachine.convertTime(StateMachine.onTime) || StateMachine.stateTimerRunning == false ) {
            StateMachine.changeWaitTime = StateMachine.convertTime(StateMachine.onTime);
            StateMachine.setLastStateChangeTime(now);
            newLevel = StateMachine.onLevel;
            setNewLevel = true;
          }
          StateMachine.setNextState(JT_ON);
          break;
          
        case OFF_TIME_MINIMAL:
          if ((StateMachine.changeWaitTime - (now - StateMachine.lastChangeTime)) < StateMachine.convertTime(StateMachine.offTime) || StateMachine.stateTimerRunning == false ) {
            StateMachine.changeWaitTime = StateMachine.convertTime(StateMachine.offTime);
            StateMachine.setLastStateChangeTime(now);
            setNewLevel = true; // offLevel is default, no need to set newLevel
          }
          StateMachine.setNextState(JT_OFF);
          break;
          
        default:
          StateMachine.keepCurrentState();   // avoid to run into a loop
          break;
      }
      StateMachine.setCurrentState(StateMachine.getNextState());  // save new state (currentState = nextState)
        
      if (setNewLevel) {
        setOutput(device, newLevel);   // checks for current level. don't set same level again
      }
    }
  }*/
  //*** state machine end ***//

  
  // handle feedback (sendInfoMessage)
  checkFeedback(device, channel);
}
