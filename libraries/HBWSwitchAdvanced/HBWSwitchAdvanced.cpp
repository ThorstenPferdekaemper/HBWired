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
  
  onTime = 0xFF;
  offTime = 0xFF;
  jumpTargets.WORD = 0;
  stateTimerRunning = false;
  stateChangeWaitTime = 0;
  lastStateChangeTime = 0;
  currentState = UNKNOWN_STATE;
};


// channel specific settings or defaults
void HBWSwitchAdvanced::afterReadConfig() {
  
  if (currentState == UNKNOWN_STATE) {
  // All off on init, but consider inverted setting
    digitalWrite(pin, config->n_inverted ? LOW : HIGH);    // 0=inverted, 1=not inverted (device reset will set to 1!)
    pinMode(pin,OUTPUT);
    currentState = JT_OFF;
  }
  else {
  // Do not reset outputs on config change (EEPROM re-reads), but update its state
    if (currentState == JT_ON)
      digitalWrite(pin, LOW ^ config->n_inverted);
    else if (currentState == JT_OFF)
      digitalWrite(pin, HIGH ^ config->n_inverted);
  }
  
  nextState = currentState; // no action for state machine needed
}


void HBWSwitchAdvanced::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  
  if (length >= 8) {  // got called with additional peering parameters -- test for correct NUM_PEER_PARAMS
    actiontype = *(data);
    uint8_t currentKeyNum = data[7];

    if ((actiontype & B00001111) >1) {   // TOGGLE_USE
      if (!stateTimerRunning && lastKeyNum != currentKeyNum) {   // do not interrupt running timer, ignore LONG_MULTIEXECUTE
        byte level;
        if ((actiontype & B00001111) == 2)   // TOGGLE_TO_COUNTER
          level = ((currentKeyNum %2 == 0) ? 0 : 200);  //  (switch ON at odd numbers, OFF at even numbers)
        else if ((actiontype & B00001111) == 3)   // TOGGLE_INVERSE_TO_COUNTER
          level = ((currentKeyNum %2 == 0) ? 200 : 0);
        else   // TOGGLE
          level = 255;
        
        setOutput(device,&level);
        nextState = currentState; // avoid state machine to run
      }
    }
    else if (lastKeyNum == currentKeyNum && !(bitRead(actiontype,5))) {
      // repeated key event, must be long press: LONG_MULTIEXECUTE not enabled
    }
    else {
      // assign values based on EEPROM layout
      onDelayTime = data[1];
      onTime = data[2];
      offDelayTime = data[3];
      offTime = data[4];
      jumpTargets.jt_hi_low[0] = data[5];
      jumpTargets.jt_hi_low[1] = data[6];

      nextState = FORCE_STATE_CHANGE; // force update
    }
    lastKeyNum = currentKeyNum;  // store key press number, to identify repeated key events
  }
  else {  // set value - no peering event, overwrite any timer //TODO check: ok to ignore absolute on/off time running? how do original devices handle this?
    //if (!stateTimerRunning)??
    setOutput(device,data);
    stateTimerRunning = false;
    nextState = currentState; // avoid state machine to run
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
    }else{   // on or off
      if (*data) {
        digitalWrite(pin, LOW ^ config->n_inverted);
        currentState = JT_ON;   // update for state machine
      }
      else {
        digitalWrite(pin, HIGH ^ config->n_inverted);
        currentState = JT_OFF;   // update for state machine
      }
    }
  }
  // Logging
  if(!nextFeedbackDelay && config->logging) {
    lastFeedbackTime = millis();
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
};


// read jump target entry - set by peering (used for state machine)
uint8_t HBWSwitchAdvanced::getNextState(uint8_t bitshift) {
  
  uint8_t nextJump = ((jumpTargets.WORD >>bitshift) & B00000111);
  
  if (nextJump == JT_ON) {
    if (onTime != 0xFF)      // not used is 0xFF
      nextJump = (actiontype & B10000000) ? ON_TIME_ABSOLUTE : ON_TIME_MINIMAL;  // on time ABSOLUTE or MINIMAL?
  }
  else if (nextJump == JT_OFF) {
    if (offTime != 0xFF)      // not used is 0xFF
      nextJump = (actiontype & B01000000) ? OFF_TIME_ABSOLUTE : OFF_TIME_MINIMAL;  // off time ABSOLUTE or MINIMAL?
  }
  
  if (stateTimerRunning && nextState == FORCE_STATE_CHANGE) { // timer still running, but update forced
    if (currentState == JT_ON)
      nextJump = (actiontype & B10000000) ? ON_TIME_ABSOLUTE : ON_TIME_MINIMAL;
    else if (currentState == JT_OFF)
      nextJump = (actiontype & B01000000) ? OFF_TIME_ABSOLUTE : OFF_TIME_MINIMAL;
  }
  return nextJump;
};


// convert time value stored in EEPROM to milliseconds (used for state machine)
uint32_t HBWSwitchAdvanced::convertTime(uint8_t timeValue) {
  uint8_t factor = timeValue & 0xC0;    // mask out factor (higest two bits)
  timeValue &= 0x3F;    // keep time value only

  // factors: 1,60,1000,6000 (last one is not used)
  switch (factor) {
    case 0:         // x1
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
  }
  return 0;
};


void HBWSwitchAdvanced::loop(HBWDevice* device, uint8_t channel) {

  unsigned long now = millis();

 //*** state machine begin ***//
  bool setNewLevel = false;

  if (((now - lastStateChangeTime > stateChangeWaitTime) && stateTimerRunning) || currentState != nextState) {

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
      case JT_ON:       // jump from on state
        nextState = getNextState(3);
        break;
      case JT_OFFDELAY:    // jump from off delay state
        nextState = getNextState(6);
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

    uint8_t newLevel = 0;   // default value. Will only be set if setNewLevel was also set 'true'
    uint8_t currentLevel;
    get(&currentLevel);
 
    if (nextState != JT_NO_JUMP_IGNORE_COMMAND) {
      
      switch (nextState) {
        case JT_ONDELAY:
          stateChangeWaitTime = convertTime(onDelayTime);
          lastStateChangeTime = now;
          stateTimerRunning = true;
          currentState = JT_ONDELAY;
          break;
          
        case JT_ON:
          newLevel = 200;
          setNewLevel = true;
          stateTimerRunning = false;
          break;
          
        case JT_OFFDELAY:
          stateChangeWaitTime = convertTime(offDelayTime);
          lastStateChangeTime = now;
          stateTimerRunning = true;
          currentState = JT_OFFDELAY;
          break;
          
        case JT_OFF:
          //newLevel = 0; // 0 is default
          setNewLevel = true;
          stateTimerRunning = false;
          break;
          
        case ON_TIME_ABSOLUTE:
          newLevel = 200;
          setNewLevel = true;
          stateChangeWaitTime = convertTime(onTime);
          lastStateChangeTime = now;
          stateTimerRunning = true;
          nextState = JT_ON;
          break;
          
        case OFF_TIME_ABSOLUTE:
          //newLevel = 0; // 0 is default
          setNewLevel = true;
          stateChangeWaitTime = convertTime(offTime);
          lastStateChangeTime = now;
          stateTimerRunning = true;
          nextState = JT_OFF;
          break;
          
        case ON_TIME_MINIMAL:
          newLevel = 200;
          setNewLevel = true;
          if (now - lastStateChangeTime < convertTime(onTime)) {
            stateChangeWaitTime = convertTime(onTime);
            lastStateChangeTime = now;
            stateTimerRunning = true;
          }
          nextState = JT_ON;
          break;
          
        case OFF_TIME_MINIMAL:
          //newLevel = 0; // 0 is default
          setNewLevel = true;
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
      currentState = (currentLevel ? JT_ON : JT_OFF );    // get current level and update state // TODO: actually needed? or keep for robustness?
      nextState = currentState;   // avoid to run into a loop
    }
    if (currentLevel != newLevel && setNewLevel) {   // check for current level. don't set same level again
      setOutput(device,&newLevel);
      setNewLevel = false;
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

