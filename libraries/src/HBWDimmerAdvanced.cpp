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
* Last updated: 26.01.2025
*/

#include "HBWDimmerAdvanced.h"

// Dimming channel
HBWDimmerAdvanced::HBWDimmerAdvanced(uint8_t _pin, hbw_config_dim* _config)
{
  pin = _pin;
  config = _config;
  oldOnLevel = 0;
  currentLevel = 0;
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
  if (length == NUM_PEER_PARAMS +2)  // got called with additional peering parameters -- test for correct array lenght
  {
    s_peering_list* peeringList;
    peeringList = (s_peering_list*)data;
    uint8_t currentKeyNum = data[NUM_PEER_PARAMS];
    bool sameLastSender = data[NUM_PEER_PARAMS +1];

    if ((lastKeyNum == currentKeyNum && sameLastSender) && !peeringList->LongMultiexecute)
      return;  // repeated long press reveived, but LONG_MULTIEXECUTE not enabled
    // TODO: check behaviour for LONG_MULTIEXECUTE & TOGGLE... (long toggle?)

	// hbwdebug(F("\nonDelayTime "));hbwdebug(peeringList->onDelayTime);hbwdebug(F("\n"));
	// hbwdebug(F("onTime "));hbwdebug(peeringList->onTime);hbwdebug(F("\n"));
	// hbwdebug(F("onTimeMin "));hbwdebug(peeringList->isOnTimeMinimal());hbwdebug(F("\n"));
	// hbwdebug(F("offDelayTime "));hbwdebug(peeringList->offDelayTime);hbwdebug(F("\n"));
	// hbwdebug(F("offDelayBlink "));hbwdebug(peeringList->offDelayBlink);hbwdebug(F("\n"));
	// hbwdebug(F("offDelayStep "));hbwdebug(peeringList->offDelayStep*2);hbwdebug(F("\n"));
	// hbwdebug(F("offTime "));hbwdebug(peeringList->offTime);hbwdebug(F("\n"));
	// hbwdebug(F("offTimeMin "));hbwdebug(peeringList->isOffTimeMinimal());hbwdebug(F("\n"));
	// hbwdebug(F("offLevel "));hbwdebug(peeringList->offLevel);hbwdebug(F("\n"));
	// hbwdebug(F("onLevel "));hbwdebug(peeringList->onLevel);hbwdebug(F("\n"));
	// hbwdebug(F("onLevelPrio "));hbwdebug(peeringList->onLevelPrio);hbwdebug(F("\n"));
	// hbwdebug(F("onMinLevel "));hbwdebug(peeringList->onMinLevel);hbwdebug(F("\n"));
	// hbwdebug(F("rStartStep "));hbwdebug(peeringList->rampStartStep);hbwdebug(F("\n"));

    if (peeringList->actionType == JUMP_TO_TARGET)
    {
      hbwdebug(F("jumpToTarget\n"));
      s_jt_peering_list* jtPeeringList;
      jtPeeringList = (s_jt_peering_list*)&data[PEER_PARAM_JT_START];
      // jumpToTarget(device, peeringList);
      jumpToTarget(device, peeringList, jtPeeringList);
    }
    else if (peeringList->actionType >= TOGGLE_TO_COUNTER && peeringList->actionType <= TOGGLEDIM_INVERS_TO_COUNTER)
    {
      uint8_t nextState = currentState;
      uint8_t newlevel = currentLevel;
     hbwdebug(F("kNum:"));hbwdebug(currentKeyNum);hbwdebug(F(" "));
      switch (peeringList->actionType) {
      case TOGGLE_TO_COUNTER:
         hbwdebug(F("TOGGLE_TO_C\n"));  // switch ON at odd numbers, OFF at even numbers
         nextState = (currentKeyNum & 0x01) == 0x01 ? JT_RAMPON : JT_RAMPOFF;
         break;
       case TOGGLE_INVERS_TO_COUNTER:
        hbwdebug(F("TOGGLE_INV_TO_C\n"));  // switch OFF at odd numbers, ON at even numbers
        nextState = (currentKeyNum & 0x01) == 0x00 ? JT_RAMPON : JT_RAMPOFF;
        break;
      case UPDIM:
        newlevel = dimUpDown(peeringList, DIM_UP);
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;
        break;
      case DOWNDIM:
        newlevel = dimUpDown(peeringList, DIM_DOWN);
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;
        break;
      case TOGGLEDIM:
        newlevel = (dimmingDirectionUp ^ (lastKeyNum == currentKeyNum && sameLastSender)) ? dimUpDown(peeringList, DIM_DOWN) : dimUpDown(peeringList, DIM_UP);
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;
        break;
      case TOGGLEDIM_TO_COUNTER:
        newlevel = (currentKeyNum & 0x01) ? dimUpDown(peeringList, DIM_UP) : dimUpDown(peeringList, DIM_DOWN);  // dim up at odd numbers, dim down at even numbers
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;
        break;
      case TOGGLEDIM_INVERS_TO_COUNTER:
        newlevel = (currentKeyNum & 0x01) ? dimUpDown(peeringList, DIM_DOWN) : dimUpDown(peeringList, DIM_UP);  // up/down inverse
        nextState = (newlevel >= peeringList->onMinLevel) ? JT_ON : JT_OFF;
        break;
      default: break;
      }
      destLevel = newlevel;
      uint32_t delay = getDelayForState(nextState, peeringList);  // get on/off time also for toggle & dim actions
      setState(device, nextState, delay, peeringList);  // dimUpDown will fake current state, so setState will always set newlevel
    }
    
    lastKeyNum = currentKeyNum;  // store key press number, to identify repeated key events
  }
  else {  // set value - no peering event, overwrite any timer
    // manual set ON/OFF will be without timer (i.e. DELAY_INFINITE) - which can only be changed by absolute time peering
    
    memset(stateParamList, 0, sizeof(*stateParamList));  // clear stored peering Param List on manual SET()
    
    uint8_t newState;
    if (*(data) > ON_OLD_LEVEL) {   // use toggle for any other value (202 and greater)
      // // toggle via ON/OFF RAMP
      setState(device, ((currentState == JT_OFF || currentState == JT_ONDELAY || currentState == JT_RAMPOFF) ? JT_RAMPON : JT_RAMPOFF), DELAY_INFINITE);
    }
    else {
      if (*(data) == ON_OLD_LEVEL) {   // on with last on-value      
        destLevel = oldOnLevel;
        newState = (destLevel == 0 ? JT_OFF : JT_ON);
      }
      else {
        destLevel = *(data);
        newState = (destLevel == 0 ? JT_OFF : JT_ON);  // set on or off
      }
      setState(device, newState, DELAY_INFINITE);  // will not set same state again, so below setOutput() will handle new level in same state
      setOutput(device, destLevel);  // nothing happens if old and new level are the same
    }
    stateParamList->onLevelPrio = ON_LEVEL_PRIO_HIGH;
    // is onLevelPrio HIGH actually needed? It has no effect to OFF state change, but new ON values will only be accepted with high prio and absolute time. ok?
  }
};


/* private function - returns the new level (0...200) to be set */
uint8_t HBWDimmerAdvanced::dimUpDown(s_peering_list* _peeringList, bool dimUp)
{
  uint8_t level = currentLevel;  // keep current level if we cannot dim up or down anymore
  // limits: dimStep 15% (level 30) + currentLevel 200 max
  
  if (dimUp) {  // UPDIM
    // hbwdebug(F("dimUp>"));
    level = currentLevel + (_peeringList->dimStep *2);
    if (level < _peeringList->dimMaxLevel) {
      if (level > _peeringList->offLevel && level < _peeringList->onMinLevel) {
        level = _peeringList->onMinLevel;  // dim to on level / ON
      }
    }
    else {
      level = _peeringList->dimMaxLevel;
    }
  }
  else {  // DOWNDIM
    // hbwdebug(F("dimDown<"));
    level = currentLevel - ((_peeringList->dimStep *2) < currentLevel ? (_peeringList->dimStep *2) : currentLevel);
    if (level < _peeringList->dimMinLevel) {
      level = _peeringList->dimMinLevel;
    }
    if (level < _peeringList->onMinLevel) {
      level = _peeringList->offLevel;  // dim to off level / OFF
    }
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
  
  // scale to % - according to pwm_range setting. Using 3 bits in channel XML config. Options in XML must align here...
  static const uint16_t newValueMax[7] = {
    PWM_MAX_40PCT, PWM_MAX_50PCT, PWM_MAX_60PCT, PWM_MAX_70PCT, PWM_MAX_80PCT, PWM_MAX_90PCT, PWM_MAX_100PCT
  };  // use factor 10 to avoid float, divide by 10 when calling analogWrite()!

  uint8_t newValueMin = 0;
  if (!config->voltage_default) newValueMin = 255; // Factor 10! Set 25.5 min output level (needs 0.5-5V for 1-10V mode)
  
  if (_newLevel > currentLevel)  dimmingDirectionUp = true;
  else                           dimmingDirectionUp = false;
  
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


/* standard public function - called by main loop for every channel in sequential order */
void HBWDimmerAdvanced::loop(HBWDevice* device, uint8_t channel)
{
  if (setDestLevelOnce) {  // single level changes, before timer starts or ends
    setDestLevelOnce = false;
    setOutput(device, destLevel);
  }
  
  unsigned long now = millis();

 //*** state machine begin ***//

  if ((now - lastChangeTime > changeWaitTime) && stateTimerRunning)
  {
    stopStateChangeTime();  // time was up, so don't come back into here - new valid timer should be set - or resumed below
    uint8_t newLevel = currentLevel;
	
   // #ifdef DEBUG_OUTPUT
     // hbwdebug(F("chan:"));
     // hbwdebughex(channel);
   // #endif
   
    // JT_OFFDELAY blink - calculate new time & level. Level is set by above up/down "loop"
    // rampStep is level change (offDelayStep); offDlyBlinkCounter is the number of blink times
    if (offDlyBlinkCounter) {// && currentState == JT_OFFDELAY) {
      if (dimmingDirectionUp == false) {
        destLevel = currentLevel + rampStep;
        changeWaitTime = stateParamList->offDelayOldTime *1000;
        if (offDlyBlinkCounter > 0) { offDlyBlinkCounter--; }  // end at initial level
      }
      else {
        destLevel = currentLevel - rampStep;
        changeWaitTime = stateParamList->offDelayNewTime *1000;
      }
    }
    //TODO check if the last blink should be the remaining time? this need to wait another waitTime, but without level changes... !?    // if (offDlyBlinkCounter == 1) changeWaitTime = convertTime(stateParamList->offDelayTime) - ((uint32_t)(stateParamList->offDelayNewTime + stateParamList->offDelayOldTime) *1000) *((convertTime(stateParamList->offDelayTime) /1000) / (stateParamList->offDelayNewTime + stateParamList->offDelayOldTime));
   
    if (currentLevel != destLevel) {
    // hbwdebug(F(" loop u/d "));
      uint8_t diff;
      if (currentLevel > destLevel ) { // dim down
        diff = currentLevel - destLevel;
        newLevel = (currentLevel - (diff < rampStep ? diff : rampStep));
      }
      else { // dim up
        diff = destLevel - currentLevel;
        newLevel = (currentLevel + (diff < rampStep ? diff : rampStep));
      }
      // hbwdebug(F("nlvl:"));hbwdebug(newLevel);hbwdebug(F(" dst:"));hbwdebug(destLevel);hbwdebug(F(" diff:"));hbwdebug(diff);
    }
    
    // we catch our destination level -> go to next state
    if (currentLevel == destLevel) {
    // hbwdebug(F(" loop next "));
      uint8_t nextState = getNextState();
      uint32_t delay = getDelayForState(nextState, stateParamList);
      setState(device, nextState, delay, stateParamList);
    }
    else { // enable again for next ramp step
      if (setOutputNoLogging(newLevel))  resetNewStateChangeTime(now);
    // hbwdebug(F(" restart! "));
    }
  }

  //*** state machine end ***//

  
  // handle feedback (sendInfoMessage)
  checkFeedback(device, channel);
}
