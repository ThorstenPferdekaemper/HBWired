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
*/

#include "HBWDimmerAdvanced.h"


// Dimming channel
HBWDimmerAdvanced::HBWDimmerAdvanced(uint8_t _pin, hbw_config_dim* _config) {
  pin = _pin;
  config = _config;
  currentValue = 0;
  oldValue = 0;
  nextFeedbackDelay = 0;
  lastFeedbackTime = 0;
  
  onTime = 0xFF;
  offTime = 0xFF;
  jumpTargets.DWORD = 0;
  stateTimerRunning = false;
  stateChangeWaitTime = 0;
  lastStateChangeTime = 0;
  currentState = UNKNOWN_STATE;
  
  rampStepCounter = 0;
  offDelaySingleStep = false;
};


// channel specific settings or defaults
void HBWDimmerAdvanced::afterReadConfig() {
  
  if (currentState == UNKNOWN_STATE) {
    // All off on init
    analogWrite(pin, LOW);
    currentState = JT_OFF;
  }
  else {
  // Do not reset outputs on config change (EEPROM re-reads), but update its state (to apply new settings)
    setOutputNoLogging(&currentValue);
  }
  nextState = currentState; // no action for state machine needed
};


/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWDimmerAdvanced::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  
  if (length >= NUM_PEER_PARAMS) {  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
    //peerParamActionType = *(data);
    peerParamActionType.byte = *(data);
    uint8_t currentKeyNum = data[NUM_PEER_PARAMS];

// TODO: add check for OnLevelPrio (currentOnLevelPrio && (onLevelPrio & currentOnLevelPrio))
    //if ((peerParamActionType & BITMASK_ActionType) >1) {   // ACTION_TYPE
    if ((peerParamActionType.element.actionType) >1) {   // ACTION_TYPE
      
      // do not interrupt running timer. First key press goes here, repeated press only when LONG_MULTIEXECUTE is enabled
      if ((!stateTimerRunning) && ((lastKeyNum != currentKeyNum) || (lastKeyNum == currentKeyNum && peerParamActionType.element.longMultiexecute))) {
        byte level = currentValue;

        //switch (peerParamActionType & BITMASK_ActionType) {
        switch (peerParamActionType.element.actionType) {
          case 2:  // TOGGLE_TO_COUNTER
            level = (currentKeyNum %2 == 0) ? data[D_POS_offLevel] : data[D_POS_onLevel];  // switch ON at odd numbers, OFF at even numbers
            break;
          case 3:  // TOGGLE_INVERSE_TO_COUNTER
            level = (currentKeyNum %2 == 0) ? data[D_POS_onLevel] : data[D_POS_offLevel];  // ON/OFF inverse
            break;
          case 4:  // UPDIM
            level = dimUpDown(data, DIM_UP);
            break;
          case 5:  // DOWNDIM
            level = dimUpDown(data, DIM_DOWN);
            break;
          case 6:  // TOGGLEDIM
            // check for currentState? - dim down initially [what defines "initially"?] when state is ON, else DOWN - but still alternate by currentKeyNum????
            // or - do now downDim, if already below onMinLevel && do not upDim when already at onLevel. (currentKeyNum %2 == toggleDimInv) toggleDimInv = true ||false???
          case 7:  // TOGGLEDIM_TO_COUNTER
            level = (currentKeyNum %2 == 0) ? dimUpDown(data, DIM_DOWN) : dimUpDown(data, DIM_UP);  // dim up at odd numbers, dim down at even numbers
            break;
          case 8:  // TOGGLEDIM_INVERS_TO_COUNTER
            level = (currentKeyNum %2 == 0) ? dimUpDown(data, DIM_UP) : dimUpDown(data, DIM_DOWN);  // up/down inverse
            break;
        }

        setOutput(device, &level);

        // keep track of the operations state
        if (currentValue >= onMinLevel)
          currentState = JT_ON;
        else
          currentState = JT_OFF;
    
        nextState = currentState; // avoid state machine to run
        
  #ifdef DEBUG_OUTPUT
    hbwdebug(F("TgDi_lvl: "));
    hbwdebug(level);
    hbwdebug(F(" aType: "));
    hbwdebug(peerParamActionType.byte);
    hbwdebug(F("\n"));
  #endif
      }
    }
    else {
      // action type: JUMP_TO_TARGET
      // assign values based on EEPROM layout (XML and link/peering details must match!)
      onDelayTime = data[D_POS_onDelayTime];
      onTime = data[D_POS_onTime];
      offDelayTime = data[D_POS_offDelayTime];
      offTime = data[D_POS_offTime];
      jumpTargets.jt_hi_low[0] = data[D_POS_jumpTargets0];
      jumpTargets.jt_hi_low[1] = data[D_POS_jumpTargets1];
      jumpTargets.jt_hi_low[2] = data[D_POS_jumpTargets2];
      offLevel = data[D_POS_offLevel];
      onMinLevel = data[D_POS_onMinLevel];
      onLevel = data[D_POS_onLevel];
      peerConfigParam.byte = data[D_POS_peerConfigParam];
      rampOnTime = data[D_POS_rampOnTime];
      rampOffTime = data[D_POS_rampOffTime];
//      dimMinLevel = data[D_POS_dimMinLevel];
//      dimMaxLevel = data[D_POS_dimMaxLevel];
      peerConfigStep.byte = data[D_POS_peerConfigStep];
      peerConfigOffDtime.byte = data[D_POS_peerConfigOffDtime];
      
      nextState = FORCE_STATE_CHANGE; // force update
    }
    lastKeyNum = currentKeyNum;  // store key press number, to identify repeated key events
  }
  else {  // set value - no peering event, overwrite any timer //TODO check: ok to ignore absolute on/off time running? how do original devices handle this?
    //if (!stateTimerRunning)??
    stateTimerRunning = false;
    setOutput(device, data);
    
    // keep track of the operations state
    if (currentValue)
      currentState = JT_ON;
    else
      currentState = JT_OFF;
    
    nextState = currentState; // avoid state machine to run
  }
};


/* private function - returns the new level (0...200) to be set */
uint8_t HBWDimmerAdvanced::dimUpDown(uint8_t const * const data, boolean dimUp) {
  
  byte level = currentValue;  // keep current level if we cannot dim up or down anymore
  peerConfigParam.byte = data[D_POS_peerConfigParam];
  peerConfigStep.byte = data[D_POS_peerConfigStep];
  
  // UPDIM
  if (dimUp) {
    if (currentValue < data[D_POS_dimMaxLevel]) {
      level = currentValue + (peerConfigStep.element.dimStep *2);
      if (level >= data[D_POS_dimMaxLevel])
        level = data[D_POS_dimMaxLevel];
      if (level > data[D_POS_offLevel] && level < data[D_POS_onMinLevel]) {
        level = data[D_POS_onMinLevel];
      }
    }
    else {
      level = data[D_POS_dimMaxLevel];  // TODO: only set, if current prio is low or new value is with high prio
    }
  }
  // DOWNDIM
  else {
    if (currentValue > data[D_POS_dimMinLevel]) {
      level = currentValue - (peerConfigStep.element.dimStep *2);
      
      if (level <= data[D_POS_dimMinLevel])
        level = data[D_POS_dimMinLevel];
      
      if (level < data[D_POS_onMinLevel])
        level = data[D_POS_offLevel];
    }
    else
      level = data[D_POS_dimMinLevel];  // TODO: only set, if current prio is low or new value is with high prio
  }
  
  return level;
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDimmerAdvanced::get(uint8_t* data) {
  (*data) = currentValue;
	return 1;
};


/* private function - sets/operates the actual outputs. Sets timer for logging/i-message */
void HBWDimmerAdvanced::setOutput(HBWDevice* device, uint8_t const * const data) {

#ifdef DEBUG_OUTPUT
  hbwdebug(F("l-"));    // indicate set output _with_ logging
#endif

  uint8_t dataNew = (*data);
  
  if (dataNew >= ON_LEVEL_USE_OLD_VALUE) {  // use OLD_LEVEL for ON_LEVEL
    if (oldValue > onMinLevel)
      dataNew = oldValue;
    else
      dataNew = onMinLevel; // TODO: what to use? onMinLevel or offLevel?
  }
  
  if (currentValue != dataNew) {  // only set output and send i-message if value was changed
    setOutputNoLogging(&dataNew);
    
    // Logging
    if(!nextFeedbackDelay && config->logging) {
      lastFeedbackTime = millis();
      nextFeedbackDelay = device->getLoggingTime() * 100;
    }
  }
};


/* private function - sets/operates the actual outputs. No logging/i-message! */
void HBWDimmerAdvanced::setOutputNoLogging(uint8_t const * const data) {
  
  if (config->pwm_range >= 1) {   // 0=disabled
    //                        scale to 40%   50%   60%   70%   80%   90%  100%
    static uint16_t newValueMax[7] = {1020, 1275, 1530, 1785, 2040, 2295, 2550};  // to avoid float, must devide by 10 when calling analogWrite()!
    uint8_t newValueMin = 0;
    uint8_t newValue;
    
    if (!config->voltage_default) newValueMin = 255; // Factor 10! Set 25.5 min output level (need 0.5-5V for 1-10V mode)
    
    if (*data <= 200) {  // set value
      newValue = (map(*data, 0, 200, newValueMin, newValueMax[config->pwm_range -1])) /10;  // map 0-200 into correct PWM range
      currentValue = (*data);
    }
    analogWrite(pin, newValue);
    
#ifdef DEBUG_OUTPUT
  hbwdebug(F("setPWM: "));
  hbwdebug(newValue);
  hbwdebug(F(" max: "));
  hbwdebug((newValueMax[config->pwm_range -1])/10);
  hbwdebug(F("\n"));
#endif
  }
};


/* read jump target entry - set by peering (used for state machine) */
uint8_t HBWDimmerAdvanced::getNextState(uint8_t bitshift) {
  
  uint8_t nextJump = ((jumpTargets.DWORD >>bitshift) & B00000111);
  
  if (nextJump == JT_ON) {
    if (onTime != 0xFF)      // not used is 0xFF
      //nextJump = (peerParamActionType & B10000000) ? ON_TIME_ABSOLUTE : ON_TIME_MINIMAL;  // on time ABSOLUTE or MINIMAL?
      nextJump = (peerParamActionType.element.onTimeMode) ? ON_TIME_ABSOLUTE : ON_TIME_MINIMAL;  // on time ABSOLUTE or MINIMAL?
  }
  else if (nextJump == JT_OFF) {
    if (offTime != 0xFF)      // not used is 0xFF
      //nextJump = (peerParamActionType & B01000000) ? OFF_TIME_ABSOLUTE : OFF_TIME_MINIMAL;  // off time ABSOLUTE or MINIMAL?
      nextJump = (peerParamActionType.element.offTimeMode) ? OFF_TIME_ABSOLUTE : OFF_TIME_MINIMAL;  // off time ABSOLUTE or MINIMAL?
  }
  if (stateTimerRunning && nextState == FORCE_STATE_CHANGE) { // timer still running, but update forced
    if (currentState == JT_ON)
      //nextJump = (peerParamActionType & B10000000) ? ON_TIME_ABSOLUTE : ON_TIME_MINIMAL;
      nextJump = (peerParamActionType.element.onTimeMode) ? ON_TIME_ABSOLUTE : ON_TIME_MINIMAL;
    else if (currentState == JT_OFF)
      //nextJump = (peerParamActionType & B01000000) ? OFF_TIME_ABSOLUTE : OFF_TIME_MINIMAL;
      nextJump = (peerParamActionType.element.offTimeMode) ? OFF_TIME_ABSOLUTE : OFF_TIME_MINIMAL;
  }
  return nextJump;
};


/* convert time value stored in EEPROM to milliseconds (used for state machine) */
uint32_t HBWDimmerAdvanced::convertTime(uint8_t timeValue) {
  
  uint8_t factor = timeValue & 0xC0;    // mask out factor (higest two bits)
  timeValue &= 0x3F;    // keep time value only

  // factors: 1,60,1000,6000 (last one is not used)
  switch (factor) {
    case 0:          // x1
      return (uint32_t)timeValue *1000;
      break;
    case 64:         // x60
      return (uint32_t)timeValue *60000;
      break;
    case 128:        // x1000
      return (uint32_t)timeValue *1000000;
      break;
//    case 192:        // not used value
//      return 0; // TODO: check how to handle this properly, what does on/off time == 0 mean? always on/off??
//      break;
//TODO: check; case 255: // not used value?
  }
  return 0;
};


/* private function - sets all variables for On/OffRamp. "rampStepCounter" must be set prior calling this function */
void HBWDimmerAdvanced::prepareOnOffRamp(uint8_t rampTime) {

  stateChangeWaitTime = convertTime(rampTime);
  
  if (stateChangeWaitTime != 255 && stateChangeWaitTime != 0 && rampStepCounter) {   // time == 0xFF when not used
  // TODO: && onLevel > onMinLevel ???
    //rampStepCounter = (onLevel - onMinLevel) *10;

    if (stateChangeWaitTime > rampStepCounter * (RAMP_MIN_STEP_WIDTH/10)) {
      rampStep = 10;      // factor 10
      stateChangeWaitTime = stateChangeWaitTime / (rampStepCounter /10);
    }
    else {
      rampStep = rampStepCounter / (stateChangeWaitTime / RAMP_MIN_STEP_WIDTH);  // factor 10
      stateChangeWaitTime = RAMP_MIN_STEP_WIDTH;
    }
  }
  else {
    rampStepCounter = 0;
    rampStep = 0;
  }
  
  lastStateChangeTime = millis();
  stateTimerRunning = true;

#ifdef DEBUG_OUTPUT
  hbwdebug(F("stChgWaitTime: "));
  hbwdebug(stateChangeWaitTime);
  hbwdebug(F(" RmpSt: "));
  hbwdebug(rampStep);
  hbwdebug(F("\n"));
#endif
};


/* standard public function - called by main loop for every channel in sequential order */
void HBWDimmerAdvanced::loop(HBWDevice* device, uint8_t channel) {

 //*** state machine begin ***//
  
  unsigned long now = millis();

  if (((now - lastStateChangeTime > stateChangeWaitTime) && stateTimerRunning) || currentState != nextState) {

    // on / off ramp
    if (rampStepCounter && (currentState == JT_RAMP_ON || currentState == JT_RAMP_OFF) && nextState != FORCE_STATE_CHANGE) {
      uint8_t rampNewValue;
      lastStateChangeTime = now;
      
      if (currentState == JT_RAMP_ON)
        rampNewValue = (onLevel *10 - rampStepCounter) /10;
      else
        rampNewValue = (rampStepCounter /10) + onMinLevel;
      
      setOutputNoLogging(&rampNewValue);  // never send i-message here. State change to on or off will do it
  
      if (rampStep > rampStepCounter)
        rampStepCounter = 0;
      else
        rampStepCounter -= rampStep;

//#ifdef DEBUG_OUTPUT
//  hbwdebug(F("RAMP-ct: "));
//  hbwdebug(rampStepCounter);
//  hbwdebug(F("\n"));
//#endif
    }
    // off delay blink
    else if (currentState == JT_OFFDELAY && rampStepCounter) {
      uint8_t newValue;
      lastStateChangeTime = now;
      
      if (offDelayNewTimeActive) {
        newValue = currentValue - (peerConfigStep.element.offDelayStep *4);
        stateChangeWaitTime = peerConfigOffDtime.element.offDelayOldTime *1000;
        offDelayNewTimeActive = false;
      }
      else {
        newValue = currentValue + (peerConfigStep.element.offDelayStep *4);
        stateChangeWaitTime = peerConfigOffDtime.element.offDelayNewTime *1000;
        offDelayNewTimeActive = true;
        rampStepCounter--;
      }
#ifdef DEBUG_OUTPUT
  hbwdebug(F("BLINK "));
#endif

      setOutputNoLogging(&newValue);
      
      if (offDelaySingleStep) {
        offDelaySingleStep = false;
        rampStepCounter = 0;
        stateChangeWaitTime = convertTime(offDelayTime);
        // TODO: do not set lastStateChangeTime = now; for this case?
      }
    }
    else {
      bool setNewLevel = false;
      rampStepCounter = 0;  // clear counter, when state change was forced TODO: needed?
      
      if (currentState == nextState)  // no change to state, so must be time triggered
        stateTimerRunning = false;

  #ifdef DEBUG_OUTPUT
    hbwdebug(F("chan:"));
    hbwdebughex(channel);
    hbwdebug(F(" cs:"));
    hbwdebughex(currentState);
  #endif
      
      // check next jump from current state
      switch (currentState) {
        case JT_ONDELAY:      // jump from on delay state
          nextState = getNextState(0);
          break;
        case JT_RAMP_ON:       // jump from ramp on state
          nextState = getNextState(12);
          break;
        case JT_ON:       // jump from on state
          //if (currentValue > onMinLevel) oldValue = currentValue;  // save current on value before off ramp or switching off
          oldValue = currentValue;  // save current on value before off ramp or switching off
          nextState = getNextState(3);
          break;
        case JT_OFFDELAY:    // jump from off delay state
          nextState = getNextState(6);
          break;
        case JT_RAMP_OFF:       // jump from ramp off state
          nextState = getNextState(15);
          break;
        case JT_OFF:      // jump from off state
          nextState = getNextState(9);
          break;
      }
  #ifdef DEBUG_OUTPUT
    hbwdebug(F(" ns:"));
    hbwdebughex(nextState);
    hbwdebug(F("\n"));
  #endif
  
      uint8_t newLevel = offLevel;   // default value. Will only be set if setNewLevel was also set 'true'
//      uint8_t currentLevel;
//      get(&currentLevel);
   
      if (nextState < JT_NO_JUMP_IGNORE_COMMAND) {
        
        switch (nextState) {
          
          case JT_ONDELAY:
            if (peerConfigParam.element.onDelayMode == 0)  setNewLevel = true;
            stateChangeWaitTime = convertTime(onDelayTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
            currentState = JT_ONDELAY;
            break;
  
          case JT_RAMP_ON:
    #ifdef DEBUG_OUTPUT
      hbwdebug(F("onLvl: "));
      hbwdebug(onLevel);
    #endif
            if (onLevel >= ON_LEVEL_USE_OLD_VALUE) {  // use OLD_LEVEL for ON_LEVEL
              if (oldValue > onMinLevel) {
                onLevel = oldValue;
              }
              else {
                onLevel = onMinLevel; // TODO: what to use? onMinLevel or offLevel?
                rampOnTime = 0;   // no ramp to set onMinLevel
              }
            }
    #ifdef DEBUG_OUTPUT
      hbwdebug(F(" oldVal: "));
      hbwdebug(oldValue);
      hbwdebug(F(" new onLvl: "));
      hbwdebug(onLevel);
      hbwdebug(F("\n"));
    #endif
            rampStepCounter = (onLevel - onMinLevel) *10;
            prepareOnOffRamp(rampOnTime); // rampStepCounter must be calculated before calling
            rampStepCounter -= rampStep; // reduce by one step, as we go to min. on level immediately
            newLevel = onMinLevel;
            setNewLevel = true;
            currentState = JT_RAMP_ON;
            break;
          
          case JT_ON:
            newLevel = onLevel;
            setNewLevel = true;
            stateTimerRunning = false;
            currentState = JT_ON;
            break;
            
          case JT_OFFDELAY:
            stateChangeWaitTime = convertTime(offDelayTime);
            // only reduce level, if not going below min. on level
        // TODO: check if should be forced to blink, by: onMinLevel + offDelayStep ??
            if (peerConfigStep.element.offDelayStep && (currentValue - onMinLevel) > (peerConfigStep.element.offDelayStep *4)) {
              offDelayNewTimeActive = true;
              stateChangeWaitTime = 0; // start immediately
              
              if (peerConfigParam.element.offDelayBlink) {  // calculate total level changes for off delay blink duration
                rampStepCounter = ((convertTime(offDelayTime) /1000) / (peerConfigOffDtime.element.offDelayNewTime + peerConfigOffDtime.element.offDelayOldTime));
              }
              else {  // only reduce level, no blink
                rampStepCounter = 1;
                offDelaySingleStep = true;
              }
            }
            lastStateChangeTime = now;
            stateTimerRunning = true;
            currentState = JT_OFFDELAY;
            break;
            
          case JT_RAMP_OFF:
            if (currentValue > onMinLevel)
              rampStepCounter = (currentValue - onMinLevel) *10;  // do not create overflow by subtraction
            else
              rampStepCounter = 0;
            prepareOnOffRamp(rampOffTime);
            currentState = JT_RAMP_OFF;
            break;
            
          case JT_OFF:
            setNewLevel = true; // offLevel is default, no need to set newLevel
            stateTimerRunning = false;
            currentState = JT_OFF;
            break;
            
          case ON_TIME_ABSOLUTE:
            newLevel = onLevel;
            setNewLevel = true;
            stateChangeWaitTime = convertTime(onTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
            nextState = JT_ON;
            break;
            
          case OFF_TIME_ABSOLUTE:
            setNewLevel = true; // offLevel is default, no need to set newLevel
            stateChangeWaitTime = convertTime(offTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
            nextState = JT_OFF;
            break;
            
          case ON_TIME_MINIMAL:
            newLevel = onLevel;
            setNewLevel = true;
            if (now - lastStateChangeTime < convertTime(onTime)) {
              stateChangeWaitTime = convertTime(onTime);
              lastStateChangeTime = now;
              stateTimerRunning = true;
            }
            nextState = JT_ON;
            break;
            
          case OFF_TIME_MINIMAL:
            setNewLevel = true; // offLevel is default, no need to set newLevel
            if (now - lastStateChangeTime < convertTime(offTime)) {
              stateChangeWaitTime = convertTime(offTime);
              lastStateChangeTime = now;
              stateTimerRunning = true;
            }
            nextState = JT_OFF;
            break;
        }
      }
      else {  // NO_JUMP_IGNORE_COMMAND
        nextState = currentState;   // avoid to run into a loop
      }
      //if (currentLevel != newLevel && setNewLevel) {   // check for current level. don't set same level again
      if (setNewLevel) {
        setOutput(device, &newLevel);   // checks for current level. don't set same level again
        setNewLevel = false;
      }
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
  	nextFeedbackDelay = 250;  // try again later, but insert a small delay
  }
  else {
  	nextFeedbackDelay = 0;
  }
}
