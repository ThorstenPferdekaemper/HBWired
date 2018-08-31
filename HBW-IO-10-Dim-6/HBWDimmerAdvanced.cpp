/* 
* HBWDimmerAdvanced
*
* Mit HBWDimmerAdvanced & HBWLinkSwitchAdvanced sind folgende Funktionen möglich:
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
  stateCangeWaitTime = 0;
  lastStateChangeTime = 0;
  currentState = UNKNOWN_STATE;
  
  rampStepCounter = 0;
};


// channel specific settings or defaults
void HBWDimmerAdvanced::afterReadConfig() {
  
  if (currentState == UNKNOWN_STATE) {
  // All off on init
    analogWrite(pin, LOW);
    currentState = JT_OFF;
  }
  else {
  // Do not reset outputs on config change (EEPROM re-reads), but update its state
    setOutputNoLogging(&currentValue);
  }
  nextState = currentState; // no action for state machine needed
};


void HBWDimmerAdvanced::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  
  if (length >= NUM_PEER_PARAMS) {  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
    //peerParamActionType = *(data);
    peerParamActionType.byte = *(data);
    uint8_t currentKeyNum = data[NUM_PEER_PARAMS];

// TODO: add check for OnLevelPrio (currentOnLevelPrio && (onLevelPrio & currentOnLevelPrio))
    //if ((peerParamActionType & BITMASK_ActionType) >1) {   // ACTION_TYPE
    if ((peerParamActionType.element.actionType) >1) {   // ACTION_TYPE
      
      // do not interrupt running timer. First key press goes here, repeated press only when LONG_MULTIEXECUTE is enabled
      //if ((!stateTimerRunning) && ((lastKeyNum != currentKeyNum) || (lastKeyNum == currentKeyNum && bitRead(peerParamActionType,5)))) {
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

//        if (level != currentValue)
        setOutput(device, &level);

        // keep track of the operations state
        if (currentValue >= onMinLevel)
          currentState = JT_ON;
        else
          currentState = JT_OFF;
    
        nextState = currentState; // avoid state machine to run
        
  #ifndef NO_DEBUG_OUTPUT
    hbwdebug(F("TgDi_lvl: "));
    hbwdebug(level);
    hbwdebug(F(" aType: "));
    hbwdebug(peerParamActionType.byte);
    hbwdebug(F("\n"));
  #endif
      }
    }
    //else if (lastKeyNum == currentKeyNum && !(peerParamActionType & B00010000)) {
    //else if (lastKeyNum == currentKeyNum && !(bitRead(peerParamActionType,5))) {
//    else if (lastKeyNum == currentKeyNum && !(peerParamActionType.element.longMultiexecute)) {
//      // repeated key event, must be long press, but longMultiexecute is not enabled... so ignore!
//      asm ("nop");
//    }
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


/* standard public function - returns length of data array. data array contains current channel reading */
uint8_t HBWDimmerAdvanced::get(uint8_t* data) {
  (*data) = currentValue;
	return 1;
};


/* private function - sets/operates the actual outputs. Sets timer for logging/i-message */
void HBWDimmerAdvanced::setOutput(HBWDevice* device, uint8_t const * const data) {

#ifndef NO_DEBUG_OUTPUT
  hbwdebug(F("l-"));    // indicate set output _with_ logging
#endif

  uint8_t dataNew = (*data);
  
  if (dataNew == 201) // use OLD_LEVEL for ON_LEVEL
    dataNew = oldValue;
  
  if (currentValue != dataNew) {
    
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
    static uint16_t newValueMax[7] = {1020, 1275, 1530, 1785, 2040, 2295, 2550};  // to avoid float, devide by 10 when calling analogWrite()!
    byte newValueMin = 0;
    byte newValue;
    
    if (!config->voltage_default) newValueMin = 255; // set 25.5 min output level (need 0.5-5V for 1-10V mode)
    
    if (*data <= 200) {  // set value
      newValue = (map(*data, 0, 200, newValueMin, newValueMax[config->pwm_range -1])) /10;  // map 0-200 into correct PWM range
      currentValue = (*data);
    }
    
    analogWrite(pin, newValue);
    
#ifndef NO_DEBUG_OUTPUT
  hbwdebug(F("set PWM: "));
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


void HBWDimmerAdvanced::prepareOnOffRamp(uint8_t rampTime) {

  stateCangeWaitTime = convertTime(rampTime);
  
  if (stateCangeWaitTime != 255 && stateCangeWaitTime != 0) {   // time == 0xFF when not used
  // TODO: && onLevel > onMinLevel ???
    //rampStepCounter = (onLevel - onMinLevel) *10;

    if (stateCangeWaitTime > rampStepCounter * (RAMP_MIN_STEP_WIDTH/10)) {
      rampStep = 10;      // factor 10
      stateCangeWaitTime = stateCangeWaitTime / (rampStepCounter /10);
    }
    else {
      rampStep = rampStepCounter / (stateCangeWaitTime / RAMP_MIN_STEP_WIDTH);  // factor 10
      stateCangeWaitTime = RAMP_MIN_STEP_WIDTH;
    }
  }
  else {
    rampStepCounter = 0;
    rampStep = 0;
  }
  
  lastStateChangeTime = millis();
  stateTimerRunning = true;

#ifndef NO_DEBUG_OUTPUT
hbwdebug(F("stCangeWaitTime: "));
hbwdebug(stateCangeWaitTime);
hbwdebug(F(" RmpSt: "));
hbwdebug(rampStep);
hbwdebug(F("\n"));
#endif
};


/* standard public function - is called by main loop for every channel in sequential order */
void HBWDimmerAdvanced::loop(HBWDevice* device, uint8_t channel) {

 //*** state machine begin ***//
  
  bool setNewLevel = false;
  unsigned long now = millis();

  if (((now - lastStateChangeTime > stateCangeWaitTime) && stateTimerRunning) || currentState != nextState) {

    if (rampStepCounter && (currentState == JT_RAMP_ON || currentState == JT_RAMP_OFF) && nextState != FORCE_STATE_CHANGE) { // TODO no need to check for currentState?? keep for robustness?
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
      
//  #ifndef NO_DEBUG_OUTPUT
//    hbwdebug(F("RmpStC:"));
//    hbwdebug(rampStepCounter);
//    hbwdebug(F("\n"));
//  #endif
    }
    else {
      
      if (currentState == nextState)  // no change to state, so must be time triggered
        stateTimerRunning = false;

  #ifndef NO_DEBUG_OUTPUT
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
  #ifndef NO_DEBUG_OUTPUT
    hbwdebug(F(" ns:"));
    hbwdebughex(nextState);
    hbwdebug(F("\n"));
  #endif
  
      uint8_t newLevel = offLevel;   // default value. Will only be set if setNewLevel was also set 'true'
//      uint8_t currentLevel;
//      get(&currentLevel);
   
      if (nextState != JT_NO_JUMP_IGNORE_COMMAND) {
        
        switch (nextState) {
          case JT_ONDELAY:
            if (peerConfigParam.element.onDelayMode == 0)  setNewLevel = true;
            stateCangeWaitTime = convertTime(onDelayTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
            currentState = JT_ONDELAY;
            break;
  
          case JT_RAMP_ON:
            if (onLevel == 201)  onLevel = oldValue;
            rampStepCounter = (onLevel - onMinLevel) *10;
            prepareOnOffRamp(rampOnTime);
            rampStepCounter -= rampStep; // reduce by one step, as we go to min. on level immediatly
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
            stateCangeWaitTime = convertTime(offDelayTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
            currentState = JT_OFFDELAY;
            break;
            
          case JT_RAMP_OFF:
            if (currentValue > onMinLevel)  rampStepCounter = (currentValue - onMinLevel) *10;
            else  rampStepCounter = 0;
            prepareOnOffRamp(rampOffTime);
            currentState = JT_RAMP_OFF;
            break;
            
          case JT_OFF:
            oldValue = currentValue;
            //newLevel = 0; // offLevel is default
            setNewLevel = true;
            stateTimerRunning = false;
            currentState = JT_OFF;
            break;
            
          case ON_TIME_ABSOLUTE:
            newLevel = onLevel;
            setNewLevel = true;
            stateCangeWaitTime = convertTime(onTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
            nextState = JT_ON;
            break;
            
          case OFF_TIME_ABSOLUTE:
            //newLevel = 0; // offLevel is default
            setNewLevel = true;
            stateCangeWaitTime = convertTime(offTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
            nextState = JT_OFF;
            break;
            
          case ON_TIME_MINIMAL:
            newLevel = onLevel;
            setNewLevel = true;
            if (now - lastStateChangeTime < convertTime(onTime)) {
              stateCangeWaitTime = convertTime(onTime);
              lastStateChangeTime = now;
              stateTimerRunning = true;
            }
            nextState = JT_ON;
            break;
            
          case OFF_TIME_MINIMAL:
            //newLevel = 0; // offLevel is default
            setNewLevel = true;
            if (now - lastStateChangeTime < convertTime(offTime)) {
              stateCangeWaitTime = convertTime(offTime);
              lastStateChangeTime = now;
              stateTimerRunning = true;
            }
            nextState = JT_OFF;
            break;
        }
      }
      else {  // NO_JUMP_IGNORE_COMMAND
  //      currentState = (currentLevel ? JT_ON : JT_OFF );    // get current level and update state // TODO: actually needed? or keep for robustness?
        nextState = currentState;   // avoid to run into a loop
      }
      //if (currentLevel != newLevel && setNewLevel) {   // check for current level. don't set same level again
      //if (currentValue != newLevel && setNewLevel) {   // check for current level. don't set same level again
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
    // try again later, but insert a small delay
    	nextFeedbackDelay = 250;
    }else{
    	nextFeedbackDelay = 0;
    }
}

