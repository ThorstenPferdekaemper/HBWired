/*
 * HBWValve.cpp
 *
 * Created on: 05.05.2019
 * loetmeister.de
 * 
 * Based on work by: Harald Glaser
 */
 
#include "HBWValve.h"


HBWValve::HBWValve(uint8_t _pin, hbw_config_valve* _config)
{
  config = _config;
  pin = _pin;

  outputChangeNextDelay = 8000;
  outputChangeLastTime = 600;
  onTimer = 200;
  offTimer = 200;
  stateFlags.element.upDown = 0; // 0 up; 1 down

  lastSentTime = 0;
  stateFlags.element.inAuto = false;
  valveConf.initDone = false;
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}


// channel specific settings or defaults
void HBWValve::afterReadConfig()
{
  if (config->send_max_interval == 0xFFFF)  config->send_max_interval = 150;  //TODO: check for default. 0 or high value, as we have notify already
  if (config->error_pos == 0xFF)  config->error_pos = 30;   // 15%
  if (config->valveSwitchTime == 0xFFFF)  config->valveSwitchTime = 180;  // use uint8, with 5 sec stepping? save 1 byte for x bytes of code?

   if (!valveConf.initDone) {
     valveLevel = config->error_pos;
     valveConf.firstState = true;
     valveConf.initDone = true;
   }
//  if (!stateFlags.element.inAuto || !valveConf.initDone)  // keep manual value?
//  {
//    valveLevel = config->error_pos;
//	  valveConf.initDone = true;
//  }
  
  valveConf.nextState = init_new_state();
//  stateFlags.status = !valveConf.nextState; // leave it for the notify to pickup new status
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
void HBWValve::set(HBWDevice* device, uint8_t length, uint8_t const * const data, bool setByPID)
{
  if (config->unlocked)
  {
    if ((*(data) >= 0 && *(data) <= 200) && (stateFlags.element.inAuto == MANUAL || setByPID))  // right limits only if manual or setByPID
    {
      if (valveLevel != *(data))  // set new state only if different
      {
        valveLevel < *(data) ? stateFlags.element.upDown = 1 : stateFlags.element.upDown = 0;
        valveLevel = *(data);
        valveConf.firstState = true;  // TODO: check if this should be done? it may interrupt the previous (incomplete) cycle?
        valveConf.nextState = init_new_state();
      }
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("Valve set, level: ")); hbwdebug(valveLevel);
  hbwdebug(F(" inAuto: ")); hbwdebug(stateFlags.element.inAuto); hbwdebug(F("\n"));
  #endif
    }
    else
    {
      switch (*(data))
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
      //TODO: upDown not considered here (error_pos vs valveLevel)
      stateFlags.element.inAuto ? valveLevel = config->error_pos : valveLevel;
      valveConf.firstState = true;
      valveConf.nextState = init_new_state();
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("Valve set mode, inAuto: ")); hbwdebug(stateFlags.element.inAuto); hbwdebug(F("\n"));
  #endif
    }
  }
  // send info/notify message in loop()
  if(!nextFeedbackDelay && config->logging) {
    lastFeedbackTime = millis();
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
}


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWValve::get(uint8_t* data)
{
//  uint8_t bits = 0;
  
  // state_flags
//  bits = (valveConf.status << 2) | (valveConf.inAuto << 1) | valveConf.upDown;
  
  // MSB first
  *data++ = valveLevel;
//  *data = (bits << 4);
  *data = stateFlags.byte;

  return 2;
}


bool HBWValve::getPidsInAuto()
{
  return stateFlags.element.inAuto;
}

void HBWValve::setPidsInAuto(bool newAuto)
{
  stateFlags.element.inAuto = newAuto;
}

uint8_t HBWValve::getErrorPos() {
  return config->error_pos;
}

/* standard public function - called by device main loop for every channel in sequential order */
void HBWValve::loop(HBWDevice* device, uint8_t channel)
{
	uint32_t now = millis();

  if (now - outputChangeLastTime >= outputChangeNextDelay)
  {
    //hmwdebug("nextstate: "); hmwdebug(nextState[i]); hmwdebug(" channel: "); hmwdebug(i); hmwdebug("\n");
    switchstate(valveConf.nextState);
    outputChangeLastTime = now;
  }


  // send InfoMessage if state changed, but not faster than send_max_interval
  // usually this is not used for an actor, as notify will be send on state changes - however if ...????
  if (config->send_max_interval && now - lastSentTime >= (uint32_t) (config->send_max_interval) * 1000)
  {
    static uint8_t level;
    get(&level);
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("Valve ch: ")); hbwdebug(channel); hbwdebug(F(" send: ")); hbwdebug(level/2); hbwdebug(F("%\n"));
 #endif
    device->sendInfoMessage(channel, 2, &level);    // level has 2 byte here
    lastSentTime = now;
  }
  
  // feedback trigger set?
  if (!nextFeedbackDelay)
    return;
  if (now - lastFeedbackTime < nextFeedbackDelay)
    return;
  lastFeedbackTime = now;  // at least last time of trying
  // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
  // we know that the level has 2 byte here (value & state)
  static uint8_t level;
  get(&level);
  if (device->sendInfoMessage(channel, 2, &level) == 1) {  // bus busy
  // try again later, but insert a small delay
    nextFeedbackDelay = 250;
  }
  else {
    nextFeedbackDelay = 0;
  }
}


// called by loop() with next state, if delay time has passed
void HBWValve::switchstate(int State)
{
  switch(State)
  {
    case VENTON:
//        digitalWrite(pin, ON);
      stateFlags.element.status = (false ^ config->n_inverted);
      outputChangeNextDelay = set_timer(valveConf.firstState, valveConf.nextState);
      valveConf.nextState = VENTOFF;
      valveConf.firstState = false;
    break;

    case VENTOFF:
//        digitalWrite(pin, OFF);
      stateFlags.element.status = (true ^ config->n_inverted);
      outputChangeNextDelay = set_timer(valveConf.firstState, valveConf.nextState);
      valveConf.nextState = VENTON;
      valveConf.firstState = false;
    break;
  }
  digitalWrite(pin, stateFlags.element.status);
  
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("switchtstate, "));
  State == VENTOFF ? hbwdebug(F("VENTOFF")) : hbwdebug(F("VENTON"));
  hbwdebug(F(" next delay: ")); hbwdebug(outputChangeNextDelay); hbwdebug(F("\n"));
  #endif
}


uint32_t HBWValve::set_timer(bool firstState, byte status)
{
  if (firstState == true)
    return set_peakmiddle(onTimer, offTimer);

  if (status == VENTON)  //on
    return onTimer;
  else
    return offTimer;
}


/* bisect the timer the first time */
uint32_t HBWValve::set_peakmiddle (uint32_t ontimer, uint32_t offtimer)
{
//  return first_on_or_off(ontimer, offtimer) ? (ontimer / 2) : (offtimer / 2);
  
  if (first_on_or_off(ontimer, offtimer))
    return ontimer / 2;
  else
    return offtimer / 2;
}


bool HBWValve::first_on_or_off(uint32_t ontimer, uint32_t offtimer)
{
  return (ontimer >= offtimer);
}


int HBWValve::init_new_state()
{
//int HBWValve::init_new_state(bool firstState) {
//    valveConf.firstState = firstState;
//    valveConf.nextState = init_new_state();
  onTimer = set_ontimer(valveLevel);
  offTimer = set_offtimer(onTimer);
  
  #ifdef DEBUG_OUTPUT 
  hbwdebug(F("Valve init_new_state, onTimer: "));  hbwdebug(onTimer);
  hbwdebug(F(" offTimer: "));  hbwdebug(offTimer);
  hbwdebug(F(" valveSwitchTime: "));  hbwdebug((uint32_t)config->valveSwitchTime *1000);  hbwdebug(F("\n"));
  #endif
  
  if (first_on_or_off(onTimer, offTimer)) {
    return VENTON;
  } else {
    return VENTOFF;
  }
}


uint32_t HBWValve::set_ontimer(uint8_t VentPositionRequested) {
    return ((((uint32_t)config->valveSwitchTime *10) * VentPositionRequested) / 2);
}


uint32_t HBWValve::set_offtimer(uint32_t ontimer) {
    return ((uint32_t)config->valveSwitchTime *1000 - ontimer);
}
