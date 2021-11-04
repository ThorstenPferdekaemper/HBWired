/*
 * HBWValve.cpp
 *
 * Created on: 05.05.2019
 * loetmeister.de
 * Updated: 01.11.2021
 * 
 * Based on work by: Harald Glaser
 */
 
#include "HBWValve.h"


HBWValve::HBWValve(uint8_t _pin, hbw_config_valve* _config)
{
  config = _config;
  pin = _pin;
  
  outputChangeNextDelay = OUTPUT_STARTUP_DELAY;
  outputChangeLastTime = 0;
  stateFlags.byte = 0;
  initDone = false;
  clearFeedback();
  
  digitalWrite(pin, OFF);
  pinMode(pin, OUTPUT);
}


// channel specific settings or defaults
void HBWValve::afterReadConfig()
{
  if (config->error_pos == 0xFF)  config->error_pos = 40;   // 20%
  if (config->valveSwitchTime == 0xFF || config->valveSwitchTime == 0)  config->valveSwitchTime = 18; // default 180s (factor 10!)

  if (!initDone) {
    valveLevel = config->error_pos;
    isFirstState = true;
    initDone = true;
  }
  nextState = init_new_state();
}


/*
 * set the desired Valve State in Manual Mode level = 0 - 200 like a Blind or Dimmer
 * Special values:
 * 201 - toggle automatic/manual
 * 205 - automatic (locks the channel to be controlled by linked PID channel)
 * 203 - manual (set error position 1st. Then allow any level 0...100%)
 */
/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWValve::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  set(device, length, data, false);
}

// slighlty customized set() function, to allow PID channels to set level in automatic mode
void HBWValve::set(HBWDevice* device, uint8_t length, uint8_t const * const data, bool setByPID)
{
  if (config->unlocked || setByPID)  // locked channels can still be set by PID, but are blocked for external changes
  {
    if ((*data >= 0 && *data <= 200) && (stateFlags.element.inAuto == MANUAL || setByPID))  // change level only if manual mode or setByPID
    {
      setNewLevel(device, *data);
      
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("Valve set, level: ")); hbwdebug(valveLevel);
 hbwdebug(F(" inAuto: ")); hbwdebug(stateFlags.element.inAuto); hbwdebug(F("\n"));
 #endif
    }
    else
    {
      switch (*data)
      {
        case SET_TOGGLE_AUTOMATIC:    // toogle PID mode
          stateFlags.element.inAuto = !stateFlags.element.inAuto;
          break;
        case SET_AUTOMATIC:
          stateFlags.element.inAuto = AUTOMATIC;
          break;
        case SET_MANUAL:
          stateFlags.element.inAuto = MANUAL;
          break;
      }
      setNewLevel(device, stateFlags.element.inAuto ? config->error_pos : valveLevel);
      
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("Valve set mode, inAuto: ")); hbwdebug(stateFlags.element.inAuto); hbwdebug(F("\n"));
 #endif
    }
  }
}

void HBWValve::setNewLevel(HBWDevice* device, uint8_t NewLevel)
{
  // check configured limits and adjust level to use desired switch time
  NewLevel = NewLevel > (200 - (config->limit_upper *20)) ? (200 - (config->limit_upper *20)) : NewLevel;  // 10% stepping (upper limit)
  NewLevel = NewLevel < (config->limit_lower *10) ? 0 : NewLevel;  // 5% stepping (lower limit)
  
  if (valveLevel != NewLevel)  // set new state only if different
  {
    valveLevel < NewLevel ? stateFlags.element.upDown = 1 : stateFlags.element.upDown = 0;
    valveLevel = NewLevel;
    isFirstState = true;
    nextState = init_new_state();

    // Logging
    setFeedback(device, config->logging);
  }
    //TODO: Add timestamp here (use millis() rollover? ~49 days?), to keep track of updated valve position for anti-stick?
    //any value higher than limit_lower will make the valve move...
}


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWValve::get(uint8_t* data)
{
  *data++ = valveLevel;
  *data = stateFlags.byte;

  return 2;
}


// helper functions to allow integration with PID channels (access to private variables)
bool HBWValve::getPidsInAuto()
{
  return stateFlags.element.inAuto;
}

void HBWValve::setPidsInAuto(bool newAuto)
{
  stateFlags.element.inAuto = newAuto;
}


/* standard public function - called by device main loop for every channel in sequential order */
void HBWValve::loop(HBWDevice* device, uint8_t channel)
{
  // startup handling. Only relevant if all channel remain at same error pos.
  if (outputChangeLastTime == 0 && outputChangeNextDelay == OUTPUT_STARTUP_DELAY) {
    outputChangeNextDelay = OUTPUT_STARTUP_DELAY * (channel + 1);
  }

  uint32_t now = millis();

  if (now - outputChangeLastTime >= (uint32_t)outputChangeNextDelay *100)
  {
    switchstate(nextState);
    outputChangeLastTime = now;
  }
  
  // feedback trigger set?
  checkFeedback(device, channel);
}


// called by loop() with next state, if delay time has passed
void HBWValve::switchstate(bool State)
{
  outputChangeNextDelay = set_timer(isFirstState, nextState);
  nextState = (State == VENTON ? VENTOFF : VENTON);
  stateFlags.element.status = (nextState ^ config->n_inverted);
  digitalWrite(pin, stateFlags.element.status);
  isFirstState = false;
  
 #ifdef DEBUG_OUTPUT
  hbwdebug(F("switchtstate, pin: ")); hbwdebug(pin);
  State == VENTOFF ? hbwdebug(F(" VENTOFF")) : hbwdebug(F(" VENTON"));
  hbwdebug(F(" next delay: ")); hbwdebug((uint32_t)outputChangeNextDelay *100); hbwdebug(F("\n"));
 #endif
}


uint16_t HBWValve::set_timer(bool firstState, bool Status)
{
  if (firstState == true)
    return set_peakmiddle(onTimer, offTimer);

  if (Status == VENTON)  //on
    return onTimer;
  else
    return offTimer;
}


/* bisect the timer the first time */
uint16_t HBWValve::set_peakmiddle (uint16_t ontimer, uint16_t offtimer)
{
  if (first_on_or_off(ontimer, offtimer))
    return ontimer / 2;
  else
    return offtimer / 2;
}


bool HBWValve::first_on_or_off(uint16_t ontimer, uint16_t offtimer)
{
  return (ontimer >= offtimer);
}


bool HBWValve::init_new_state()
{
  onTimer = set_ontimer(valveLevel);
  offTimer = set_offtimer(onTimer);
  
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("Valve init_new_state, onTimer: "));  hbwdebug((uint32_t)onTimer*100);
  hbwdebug(F(" offTimer: "));  hbwdebug((uint32_t)offTimer*100);
  hbwdebug(F(" valveSwitchTime: "));  hbwdebug((uint32_t)config->valveSwitchTime *10000);  hbwdebug(F("\n"));
  #endif
  
  if (first_on_or_off(onTimer, offTimer)) {
    return VENTON;
  } else {
    return VENTOFF;
  }
}


uint16_t HBWValve::set_ontimer(uint8_t VentPositionRequested) {
    return ((((uint16_t)config->valveSwitchTime) * VentPositionRequested) / 2);
}


uint16_t HBWValve::set_offtimer(uint16_t ontimer) {
    return ((uint16_t)config->valveSwitchTime *100 - ontimer);
}
