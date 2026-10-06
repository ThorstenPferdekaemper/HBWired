/*
 * HBWValve.cpp
 *
 * Created on: 05.05.2019
 * loetmeister.de
 * Updated: 30.08.2026
 * 
 * Based on work by: Harald Glaser
 */
 
#include "HBWValve.h"


HBWValve::HBWValve(uint8_t _pin, hbw_config_valve* _config)
{
  config = _config;
  pin = _pin;
  
  valveOnLastTime = 0;
  outputChangeNextDelay = OUTPUT_STARTUP_DELAY;
  outputChangeLastTime = 0;
  antiStickCycle = false;
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
    if ( *data <= 200 && (!inAuto || setByPID))  // change level only if manual mode or setByPID
    {
      setNewLevel(device, *data);
      
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("Valve set, level: ")); hbwdebug(valveLevel);
 hbwdebug(F(" newLevel: ")); hbwdebug(*data);
 hbwdebug(F(" antiStickCycle: ")); hbwdebug(antiStickCycle);
 hbwdebug(F(" inAuto: ")); hbwdebug(inAuto); hbwdebug(F("\n"));
 #endif
    }
    else
    {
      switch (*data)
      {
        case SET_TOGGLE_AUTOMATIC:    // toogle PID mode
          inAuto = !inAuto;
          break;
        case SET_AUTOMATIC:
          inAuto = true;
          break;
        case SET_MANUAL:
          inAuto = false;
          break;
      }
      setNewLevel(device, inAuto ? valveLevel : config->error_pos);
      
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("Valve set mode, inAuto: ")); hbwdebug(inAuto); hbwdebug(F("\n"));
 #endif
    }
  }
}

void HBWValve::setNewLevel(HBWDevice* device, uint8_t NewLevel)
{
  // check configured limits and adjust level to use desired switch time
  NewLevel = NewLevel > (200 - (config->limit_upper *20)) ? (200 - (config->limit_upper *20)) : NewLevel;  // 10% stepping (upper limit)
  NewLevel = NewLevel < (config->limit_lower *10) ? 0 : NewLevel;  // 5% stepping (lower limit)
  
  if (antiStickCycle && (NewLevel < config->error_pos)) {
    // ignore new level below error_pos during anti stick cycle
    NewLevel = valveLevel;
  }
  
  if (valveLevel != NewLevel)  // set new state only if different
  {
    valveLevel < NewLevel ? goingUp = 1 : goingUp = 0;
    valveLevel = NewLevel;
    isFirstState = true;
    nextState = init_new_state();
    
    // Logging
    setFeedback(device, config->logging);
  }
}


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWValve::get(uint8_t* data)
{
  u_state_flags stateFlags;
  stateFlags.element.upDown = goingUp;
  stateFlags.element.inAuto = inAuto;
  stateFlags.element.status = outputState;
  stateFlags.element.antiStickCycle = antiStickCycle;

  *data++ = valveLevel;
  *data = stateFlags.byte;

  return 2;
}


// helper functions to allow integration with PID channels (access to private variables)
bool HBWValve::getPidsInAuto()
{
  return inAuto;
}

void HBWValve::setPidsInAuto(bool newAuto)
{
  inAuto = newAuto;
}


/* standard public function - called by device main loop for every channel in sequential order */
void HBWValve::loop(HBWDevice* device, uint8_t channel)
{
  // startup handling. Only relevant if all channel remain at same error pos.
  if (outputChangeLastTime == 0 && outputChangeNextDelay == OUTPUT_STARTUP_DELAY) {
    outputChangeNextDelay = OUTPUT_STARTUP_DELAY * (channel + 1);
  }
  
  uint32_t now = millis();

  checkAntiStick(device, &now, config->anti_stick, (config->error_pos == 0));

  if (now - outputChangeLastTime >= (uint32_t)outputChangeNextDelay *100)
  {
    outputChangeLastTime = now;
    outputChangeNextDelay = set_timer(isFirstState, nextState);
    bool oldState = nextState;
    nextState = (oldState == VENTON) ? VENTOFF : VENTON;
    if (outputChangeNextDelay != 0) {   // don't change output state for 0 delay
      outputState = (nextState ^ config->n_inverted) ? ON : OFF;
      digitalWrite(pin, outputState);
    }
    isFirstState = false;

    if (valveLevel != 0 && !antiStickCycle && (now - valveOnLastTime >= (uint32_t)config->valveSwitchTime *100))
      valveOnLastTime = now;  // consider "on" only after one valveSwitchTime cycle

  #ifdef DEBUG_OUTPUT
   hbwdebug(F("switchtstate, pin: ")); hbwdebug(pin);
   oldState == VENTOFF ? hbwdebug(F(" OFF")) : hbwdebug(F(" ON"));
   hbwdebug(F(" next delay: ")); hbwdebug((uint32_t)outputChangeNextDelay *100); hbwdebug(F("\n"));
  #endif
  }
  
  // feedback trigger set?
  checkFeedback(device, channel);
}


void HBWValve::checkAntiStick(HBWDevice* device, uint32_t* now, bool antiStickEnabled, bool channelDisabled)
{
  if (!antiStickEnabled || channelDisabled) return;
    
  if (!antiStickCycle && (*now - valveOnLastTime) >= (0xFFFFFFFFUL - (2* 3600UL * 1000UL))) {
    // valve not moved for 49 days (~2 hours before rollover of millis())
    setNewLevel(device, config->error_pos);
	valveOnLastTime = *now;
    antiStickCycle = true;
    hbwdebug(F("antiStickCycle START\n"));
    return;
  }
  
  if (antiStickCycle && (*now - valveOnLastTime) >= (uint32_t)(config->valveSwitchTime *10000UL *10)) {
    // stay in antiStickCycle for valveSwitchTime times 10 (e.g. 180 sec default -> 30 minutes cycle)
	// during this time, PID ar manual vavle level can be set - when above error_pos
    // valveOnLastTime = *now;
    antiStickCycle = false;
    hbwdebug(F("antiStickCycle END\n"));
    // PID can take over again
    return;
  }
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
  hbwdebug(F(" valveSwitchTime: "));  hbwdebug((uint32_t)config->valveSwitchTime *10000UL);  hbwdebug(F("\n"));
  #endif
  
  if (first_on_or_off(onTimer, offTimer)) {
    return VENTON;
  } else {
    return VENTOFF;
  }
}


uint16_t HBWValve::set_ontimer(uint8_t VentPositionRequested) {
    return (((uint16_t)config->valveSwitchTime * VentPositionRequested) / 2);
}


uint16_t HBWValve::set_offtimer(uint16_t ontimer) {
    return ((uint16_t)config->valveSwitchTime *100 - ontimer);
}
