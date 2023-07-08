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
* Last updated: 01.11.2021
*/

#include "HBWDimmerAdvanced.h"

// Dimming channel
HBWDimmerAdvanced::HBWDimmerAdvanced(uint8_t _pin, hbw_config_dim* _config)
{
  pin = _pin;
  config = _config;
  currentValue = 0;
  oldOnValue = 0;
  clearFeedback();
  
  StateMachine.init();

  currentOnLevelPrio = ON_LEVEL_PRIO_LOW;
  rampStepCounter = 0;
  offDelaySingleStep = false;
};


// channel specific settings or defaults
void HBWDimmerAdvanced::afterReadConfig()
{
  if (StateMachine.currentStateIs(UNKNOWN_STATE)) {
    // All off on init
    analogWrite(pin, LOW);
    StateMachine.setCurrentState(JT_OFF);
  }
  else {
  // Do not reset outputs on config change, but update its state (to apply new settings from EEPROM)
    setOutputNoLogging(currentValue);
  }
  StateMachine.keepCurrentState(); // no action for state machine needed
};


/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWDimmerAdvanced::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length >= NUM_PEER_PARAMS +2)  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
  {
    StateMachine.writePeerParamActionType(*(data));
    StateMachine.writePeerConfigParam(data[D_POS_peerConfigParam]);
    uint8_t currentKeyNum = data[NUM_PEER_PARAMS];
    bool sameLastSender = data[NUM_PEER_PARAMS +1];

    if (StateMachine.peerParam_getActionType() > JUMP_TO_TARGET)   // all ACTION_TYPEs except JUMP_TO_TARGET will be covered here
    {
      if (StateMachine.currentStateIs(JT_ON) && (currentOnLevelPrio == ON_LEVEL_PRIO_HIGH && StateMachine.peerParam_onLevelPrioIsLow())) { 
            // new low onLevelPrio should not overwrite high onLevelPrio when in "on" state
            // do nothing in this case
      }
      else {
        // do not interrupt running timer. First key press goes here, repeated press only when LONG_MULTIEXECUTE is enabled
        if ((!StateMachine.stateTimerRunning) && ((StateMachine.lastKeyNum != currentKeyNum || (StateMachine.lastKeyNum == currentKeyNum && !sameLastSender)) || (StateMachine.lastKeyNum == currentKeyNum && sameLastSender && StateMachine.peerParam_getLongMultiexecute()))) {
          
          byte level = currentValue;
  
          switch (StateMachine.peerParam_getActionType()) {
            case TOGGLE_TO_COUNTER:
              level = (currentKeyNum & 1) ? data[D_POS_onLevel] : data[D_POS_offLevel];  // switch ON at odd numbers, OFF at even numbers
              break;
            case TOGGLE_INVERS_TO_COUNTER:
              level = (currentKeyNum & 1) ? data[D_POS_offLevel] : data[D_POS_onLevel];  // ON/OFF inverse
              break;
            case UPDIM:
              level = dimUpDown(data, DIM_UP);
              break;
            case DOWNDIM:
              level = dimUpDown(data, DIM_DOWN);
              break;
            case TOGGLEDIM:
              // check for currentState? - dim down initially [what defines "initially"?] when state is ON, else DOWN - but still alternate by currentKeyNum????
              // or - do now downDim, if already below onMinLevel && do not upDim when already at onLevel. (currentKeyNum %2 == toggleDimInv) toggleDimInv = true ||false???
            case TOGGLEDIM_TO_COUNTER:  // TOGGLEDIM_TO_COUNTER
              level = (currentKeyNum & 1) ? dimUpDown(data, DIM_DOWN) : dimUpDown(data, DIM_UP);  // dim up at odd numbers, dim down at even numbers
              break;
            case TOGGLEDIM_INVERS_TO_COUNTER:
              level = (currentKeyNum & 1) ? dimUpDown(data, DIM_UP) : dimUpDown(data, DIM_DOWN);  // up/down inverse
              break;
          }
  
          setOutput(device, level);
  
          // keep track of the operations state
          if (currentValue >= StateMachine.onMinLevel)
            StateMachine.setCurrentState(JT_ON);
          else
            StateMachine.setCurrentState(JT_OFF);
  
          currentOnLevelPrio = StateMachine.peerParam_getOnLevelPrio();
          StateMachine.keepCurrentState(); // avoid state machine to run
          
  #ifdef DEBUG_OUTPUT
    hbwdebug(F("Tgl_lvl: "));
    hbwdebug(level);
    hbwdebug(F(" aType: "));
    hbwdebug(StateMachine.getPeerParamActionType());
    hbwdebug(F("\n"));
  #endif
        }
      }
    }
    else if (StateMachine.lastKeyNum == currentKeyNum && sameLastSender && !StateMachine.peerParam_getLongMultiexecute()) {
      // repeated key event for ACTION_TYPE == 1 (ACTION_TYPE == 0 already filtered by receiveKeyEvent, HBWLinkReceiver)
      // repeated long press, but LONG_MULTIEXECUTE not enabled
    }
    else if (StateMachine.absoluteTimeRunning
             && (
              (StateMachine.currentStateIs(JT_ON)
               &&
                (StateMachine.peerParam_onTimeMinimal() ||
                (currentOnLevelPrio == ON_LEVEL_PRIO_HIGH && StateMachine.peerParam_onLevelPrioIsLow() && StateMachine.peerParam_onTimeAbsolute())
               )
              ) ||
              (StateMachine.currentStateIs(JT_OFF)
               &&
                (StateMachine.peerParam_offTimeMinimal() ||
                (currentOnLevelPrio == ON_LEVEL_PRIO_HIGH && StateMachine.peerParam_onLevelPrioIsLow() && StateMachine.peerParam_offTimeAbsolute()))
              )
             )) {
        // ON/OFF_TIME_ABSOLUTE running and in ON/OFF state, but new on/off time received is TIME_MINIMAL
        // do nothing in this case
        // or ON_TIME_ABSOLUTE with HIGH onLevelPrio cannot be overwritten with ON_TIME_ABSOLUTE & LOW onLevelPrio - TODO: correct behaviour?
    }
    else  // action type: JUMP_TO_TARGET
    {
      // Assign values from peering (receiveKeyEvent), which we might need in main loop later on
	  // TODO: change peering to point to EEPROM address of active peer - read needed values on demand, not store them in memory!
	  // FIXME: move ON/OFF_TIME_MINIMAL check here? .. actually this needs full check of JT, etc.? setting the peering parameters here will overwrite all values, like offLevel, when it should not (e.g. when new onTime was rejected)
      StateMachine.onDelayTime = data[D_POS_onDelayTime];
      StateMachine.onTime = data[D_POS_onTime];
      StateMachine.offDelayTime = data[D_POS_offDelayTime];
      StateMachine.offTime = data[D_POS_offTime];
      StateMachine.jumpTargets.jt_hi_low[0] = data[D_POS_jumpTargets0];
      StateMachine.jumpTargets.jt_hi_low[1] = data[D_POS_jumpTargets1];
      StateMachine.jumpTargets.jt_hi_low[2] = data[D_POS_jumpTargets2];
      StateMachine.offLevel = data[D_POS_offLevel];
      StateMachine.onMinLevel = data[D_POS_onMinLevel];
      StateMachine.onLevel = data[D_POS_onLevel];
      rampOnTime = data[D_POS_rampOnTime];
      rampOffTime = data[D_POS_rampOffTime];
      writePeerConfigStep(data[D_POS_peerConfigStep]);
      writePeerConfigOffDtime(data[D_POS_peerConfigOffDtime]);
      
      if (StateMachine.onLevel > 0 && StateMachine.onLevel >= StateMachine.onMinLevel) {  // don't allow on_level 0 or below on_min_level // TODO: should this sanity check happen only when entering on or ramp_on state? right now it blocks all processing
        StateMachine.forceStateChange(); // force update
      }
    }
    StateMachine.lastKeyNum = currentKeyNum;  // store key press number, to identify repeated key events
  }
  else {  // set value - no peering event, overwrite any timer
    //TODO check: ok to ignore absolute on/off time running? how do original devices handle this?
    //if (!stateTimerRunning)??
    // or add peerConfigParam.element.onLevelPrio = HIGH to overwrite? then back to LOW after setOutput()??
    StateMachine.stateTimerRunning = false;
    setOutput(device, *data);
    
    // keep track of the operations state
    if (currentValue)
      StateMachine.setCurrentState(JT_ON);
    else
      StateMachine.setCurrentState(JT_OFF);
    
    StateMachine.keepCurrentState(); // avoid state machine to run
  }
};


/* private function - returns the new level (0...200) to be set */
uint8_t HBWDimmerAdvanced::dimUpDown(uint8_t const * const data, boolean dimUp)
{
  byte level = currentValue;  // keep current level if we cannot dim up or down anymore
  writePeerConfigStep(data[D_POS_peerConfigStep]);
  
  // UPDIM
  if (dimUp) {
    if (currentValue < data[D_POS_dimMaxLevel]) {
      level = currentValue + (peerParam_getDimStep() *2);
      if (level >= data[D_POS_dimMaxLevel])
        level = data[D_POS_dimMaxLevel];
      if (level > data[D_POS_offLevel] && level < data[D_POS_onMinLevel]) {
        level = data[D_POS_onMinLevel];
      }
    }
    else {
      level = data[D_POS_dimMaxLevel];
    }
  }
  // DOWNDIM
  else {
    if (currentValue > data[D_POS_dimMinLevel]) {
      level = currentValue - (peerParam_getDimStep() *2);
      if (level <= data[D_POS_dimMinLevel])
        level = data[D_POS_dimMinLevel];
      if (level < data[D_POS_onMinLevel])
        level = data[D_POS_offLevel];
    }
    else
      level = data[D_POS_dimMinLevel];
  }
  
  return level;
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDimmerAdvanced::get(uint8_t* data)
{
  state_flags stateFlags;
  stateFlags.byte = 0;
  
  if (StateMachine.currentStateIs(JT_RAMP_ON) || StateMachine.currentStateIs(JT_RAMP_OFF)) {
    stateFlags.state.upDown = dimmingDirectionUp ? STATEFLAG_DIM_UP : STATEFLAG_DIM_DOWN;
  }
  else {
    // state up or down also sets channel "working" - don't set both at the same time
    stateFlags.state.working = StateMachine.stateTimerRunning ? true : false;
  }
  
  *data++ = currentValue;
  *data = stateFlags.byte;
  
  return 2;
};


/* private function - sets/operates the actual outputs. Sets timer for logging/i-message */
void HBWDimmerAdvanced::setOutput(HBWDevice* device, uint8_t newValue)
{
#ifdef DEBUG_OUTPUT
  hbwdebug(F("l-"));    // indicate set output _with_ logging
#endif
  
  if (newValue == ON_LEVEL_USE_OLD_VALUE) {  // use OLD_LEVEL for ON_LEVEL
    if (oldOnValue > StateMachine.onMinLevel)
      newValue = oldOnValue;
    else
      newValue = StateMachine.onMinLevel; // TODO: what to use? onMinLevel or offLevel?
  }
  
  if (currentValue != newValue) {  // only set output and send i-message if value was changed
    setOutputNoLogging(newValue);
    
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
void HBWDimmerAdvanced::setOutputNoLogging(uint8_t newValue)
{
  if (config->pwm_range == 0)  return;   // 0=disabled
  if (newValue > 200)  return;  // exceeding limit
  
  //                              scale to 40%   50%   60%   70%   80%   90%  100% - according to pwm_range setting
  static const uint16_t newValueMax[7] = {1020, 1275, 1530, 1785, 2040, 2300, 2550};  // avoid float, devide by 10 when calling analogWrite()!
  uint8_t newValueMin = 0;

  if (!config->voltage_default) newValueMin = 255; // Factor 10! Set 25.5 min output level (need 0.5-5V for 1-10V mode)
  
  if (newValue > currentValue)
    dimmingDirectionUp = true;
  else
    dimmingDirectionUp = false;
  
  currentValue = newValue;

  // set value
  uint8_t newPwmValue = (map(newValue, 0, 200, newValueMin, newValueMax[config->pwm_range -1])) /10;  // map 0-200 into correct PWM range
  analogWrite(pin, newPwmValue);
  
#ifdef DEBUG_OUTPUT
  hbwdebug(F("setPWM: "));
  hbwdebug(newPwmValue);
  hbwdebug(F(" max: "));
  hbwdebug((newValueMax[config->pwm_range -1])/10);
  hbwdebug(F("\n"));
#endif
};


/* private function - sets all variables required for On/OffRamp */
void HBWDimmerAdvanced::prepareOnOffRamp(uint8_t rampTime, uint8_t level)
{
  if (rampTime != 255 && rampTime != 0 && (level > StateMachine.onMinLevel)) {   // time == 0xFF when not used  
    StateMachine.stateChangeWaitTime = StateMachine.convertTime(rampTime);
    rampStepCounter = (level - StateMachine.onMinLevel) *10;  // do not create overflow by subtraction (level > onMinLevel)
    
    if (StateMachine.stateChangeWaitTime > rampStepCounter * (RAMP_MIN_STEP_WIDTH/10)) {
      rampStep = 10;      // factor 10 (1 step = 0.5%)
      StateMachine.stateChangeWaitTime = StateMachine.stateChangeWaitTime / (rampStepCounter /10);
    }
    else {
      rampStep = rampStepCounter / (StateMachine.stateChangeWaitTime / RAMP_MIN_STEP_WIDTH);  // factor 10
      StateMachine.stateChangeWaitTime = RAMP_MIN_STEP_WIDTH;
    }
  }
  else {
    rampStepCounter = 0;
    rampStep = 0;
    StateMachine.stateChangeWaitTime = 0;
  }
  
  StateMachine.setLastStateChangeTime(millis());

#ifdef DEBUG_OUTPUT
  hbwdebug(F("stChgWaitTime: "));
  hbwdebug(StateMachine.stateChangeWaitTime);
  hbwdebug(F(" RmpStCt: "));
  hbwdebug(rampStepCounter);
  hbwdebug(F(" RmpSt: "));
  hbwdebug(rampStep);
  hbwdebug(F("\n"));
#endif
};


bool HBWDimmerAdvanced::checkOnLevelPrio(void)
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
}


/* standard public function - called by main loop for every channel in sequential order */
void HBWDimmerAdvanced::loop(HBWDevice* device, uint8_t channel) {

  unsigned long now = millis();

 //*** state machine begin ***//
  
  if (((now - StateMachine.lastStateChangeTime > StateMachine.stateChangeWaitTime) && StateMachine.stateTimerRunning) || !StateMachine.noStateChange()) {
    
    if (StateMachine.getNextState() == FORCE_STATE_CHANGE) {
      rampStepCounter = 0;  // clear counter, when state change was forced
    }
    
    // on / off ramp
    if (rampStepCounter && (StateMachine.currentStateIs(JT_RAMP_ON) || StateMachine.currentStateIs(JT_RAMP_OFF))) {
      
      uint8_t rampNewValue;
      StateMachine.lastStateChangeTime = now;
      
      if (StateMachine.currentStateIs(JT_RAMP_ON))
        rampNewValue = (StateMachine.onLevel *10 - rampStepCounter) /10;
      else
        rampNewValue = (rampStepCounter /10) + StateMachine.onMinLevel;
      
      setOutputNoLogging(rampNewValue);  // never send i-message here. State change to on or off will do it
  
      if (rampStep > rampStepCounter) rampStepCounter = 0;
      else  rampStepCounter -= rampStep;

//#ifdef DEBUG_OUTPUT
//  hbwdebug(F("RAMP-ct: "));
//  hbwdebug(rampStepCounter);
//  hbwdebug(F("\n"));
//#endif
    }
    // off delay blink
    else if (rampStepCounter && StateMachine.currentStateIs(JT_OFFDELAY)) {
      uint8_t newValue;
      
      if (offDelayNewTimeActive) {
        newValue = currentValue - (peerParam_getOffDelayStep());
        StateMachine.stateChangeWaitTime = peerParam_getOffDelayOldTime() *1000;
        offDelayNewTimeActive = false;
      }
      else {
        newValue = currentValue + (peerParam_getOffDelayStep());
        StateMachine.stateChangeWaitTime = peerParam_getOffDelayNewTime() *1000;
        offDelayNewTimeActive = true;
        rampStepCounter--;
      }
      setOutputNoLogging(newValue);
      
      if (offDelaySingleStep) {   // no blink, reduce level only once, reset counter and set wait time
        offDelaySingleStep = false;
        rampStepCounter = 0;
        StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offDelayTime);
      }
      else
        StateMachine.lastStateChangeTime = now;    // only update in blink mode
    }
    else {
      if (StateMachine.noStateChange()) {  // no change to state, so must be time triggered
        StateMachine.stateTimerRunning = false;
		//TODO: need to clear StateMachine.stateChangeWaitTime and StateMachine.lastStateChangeTime?
		// replace stateTimerRunning? Use stateChangeWaitTime == 0, as stateTimerRunning = false?
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
        case JT_RAMP_ON:       // jump from ramp on state
          StateMachine.setNextState(getJumpTarget(12));
          break;
        case JT_ON:       // jump from on state
          oldOnValue = currentValue;  // save current on value before off ramp or switching off
          currentOnLevelPrio = ON_LEVEL_PRIO_LOW;  // clear OnLevelPrio when leaving ON state
          StateMachine.setNextState(getJumpTarget(3));
          break;
        case JT_OFFDELAY:    // jump from off delay state
          StateMachine.setNextState(getJumpTarget(6));
          break;
        case JT_RAMP_OFF:       // jump from ramp off state
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
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.onDelayTime);
          StateMachine.setLastStateChangeTime(now);
          break;

        case JT_RAMP_ON:
#ifdef DEBUG_OUTPUT
  hbwdebug(F("onLvl: "));
  hbwdebug(StateMachine.onLevel);
#endif
          if (StateMachine.onLevel == ON_LEVEL_USE_OLD_VALUE) {  // use OLD_LEVEL for ON_LEVEL
            if (oldOnValue > StateMachine.onMinLevel) {
              StateMachine.onLevel = oldOnValue;
            }
            else {
              StateMachine.onLevel = StateMachine.onMinLevel; // TODO: what to use? onMinLevel or offLevel? or RAMP_START_STEP?
              rampOnTime = 0;   // no ramp to set onMinLevel
            }
          }
#ifdef DEBUG_OUTPUT
  hbwdebug(F(" oldVal: "));
  hbwdebug(oldOnValue);
  hbwdebug(F(" new onLvl: "));
  hbwdebug(StateMachine.onLevel);
  hbwdebug(F("\n"));
#endif
          prepareOnOffRamp(rampOnTime, StateMachine.onLevel);
          rampStepCounter -= rampStep; // reduce by one step, as we go to min. on level immediately
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
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offDelayTime);
          // only reduce level, if not going below min. on level
      // TODO: check if should be forced to blink, by: onMinLevel + offDelayStep ??
#ifdef DEBUG_OUTPUT
  hbwdebug(F("OffDlySt:"));
  hbwdebug(peerParam_getOffDelayStep());
#endif
          if (peerParam_getOffDelayStep() && (currentValue - StateMachine.onMinLevel) > (peerParam_getOffDelayStep())) {
            offDelayNewTimeActive = true;
            StateMachine.stateChangeWaitTime = 0; // start immediately
            
            if (StateMachine.peerParam_offDelayBlinkEnabled()) {  // calculate total level changes for off delay blink duration
              rampStepCounter = ((StateMachine.convertTime(StateMachine.offDelayTime) /1000) / (peerParam_getOffDelayNewTime() + peerParam_getOffDelayOldTime()));
#ifdef DEBUG_OUTPUT
  hbwdebug(F(" blink:"));
#endif
            }
            else {  // only reduce level, no blink
              rampStepCounter = 1;
              offDelaySingleStep = true;
#ifdef DEBUG_OUTPUT
  hbwdebug(F(" nBlink:"));
#endif
            }
          }
          StateMachine.setLastStateChangeTime(now);
#ifdef DEBUG_OUTPUT
  hbwdebug(rampStepCounter);
  hbwdebug(F("\n"));
#endif
          break;
          
        case JT_RAMP_OFF:
          prepareOnOffRamp(rampOffTime, currentValue);
          break;
          
        case JT_OFF:
          setNewLevel = true; // offLevel is default, no need to set newLevel
          StateMachine.stateTimerRunning = false;
          break;
          
        case ON_TIME_ABSOLUTE:  // ABSOLUTE time will always be applied (unless new on prio is lower than current prio)
          checkOnLevelPrio();
          newLevel = StateMachine.onLevel;
          setNewLevel = true;
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.onTime);
          StateMachine.setLastStateChangeTime(now);
          StateMachine.absoluteTimeRunning = true;
          StateMachine.setNextState(JT_ON);
          break;
          
        case OFF_TIME_ABSOLUTE:
          setNewLevel = true; // offLevel is default, no need to set newLevel
          StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offTime);
          StateMachine.setLastStateChangeTime(now);
          StateMachine.absoluteTimeRunning = true;
          StateMachine.setNextState(JT_OFF);
          break;
          
        case ON_TIME_MINIMAL:  // new MINIMAL time will only be applied if current remaining time is shorter
         // TODO: validation against remaining time (lastStateChangeTime) ist ok? or should be old onTime (stateChangeWaitTime)?
          if ((StateMachine.stateChangeWaitTime - (now - StateMachine.lastStateChangeTime)) < StateMachine.convertTime(StateMachine.onTime) || StateMachine.stateTimerRunning == false ) {
            StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.onTime);
            StateMachine.setLastStateChangeTime(now);
            newLevel = StateMachine.onLevel;
            setNewLevel = true;
          }
          StateMachine.setNextState(JT_ON);
          break;
          
        case OFF_TIME_MINIMAL:
          if ((StateMachine.stateChangeWaitTime - (now - StateMachine.lastStateChangeTime)) < StateMachine.convertTime(StateMachine.offTime) || StateMachine.stateTimerRunning == false ) {
            StateMachine.stateChangeWaitTime = StateMachine.convertTime(StateMachine.offTime);
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
  }
  //*** state machine end ***//

  
  // handle feedback (sendInfoMessage)
  checkFeedback(device, channel);
}
