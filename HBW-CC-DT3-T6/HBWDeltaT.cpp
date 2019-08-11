/*
 * HBWDeltaT.cpp
 *
 * Created on: 05.05.2019
 * loetmeister.de
 * 
 */
 
#include "HBWDeltaT.h"



HBWDeltaT::HBWDeltaT(uint8_t _pin, HBWDeltaTx* _delta_t1, HBWDeltaTx* _delta_t2, hbw_config_DeltaT* _config) :
 config(_config),
 pin(_pin),
 deltaT1(_delta_t1),
 deltaT2(_delta_t2)
{
  deltaT = 0xFF;
  outputChangeLastTime = DELTA_CALCULATION_WAIT_TIME;
  deltaCalcLastTime = MIN_CHANGE_WAIT_TIME; // wait some time to get input values
  lastFeedbackTime = 0;
  stateFlags.byte = 0;
  initDone = false;
};


HBWDeltaTx::HBWDeltaTx(hbw_config_DeltaTx* _config)
{
  config = _config;

  currentTemperature = DEFAULT_TEMP; // chan must be (t > ERROR_TEMP) to work
};


// channel specific settings or defaults
void HBWDeltaT::afterReadConfig()
{
  if (initDone == false)
  {
  // All off on init, but consider inverted setting
    digitalWrite(pin, config->n_inverted ? LOW : HIGH);   // 0=inverted, 1=not inverted
    pinMode(pin,OUTPUT);
    currentState = OFF;
    nextState = currentState;  //(if key support for HBWDeltaT; TODO: reset peered actors???)
  }
  else {
  // Do not reset outputs on config change (EEPROM re-reads), but update its state
    digitalWrite(pin, !currentState ^ config->n_inverted);
  }
  initDone = true;
  
  // channel is disabled by default. Valid values must be set when being enabled...
//  if (config->deltaHys == 31)  config->deltaHys = 6; // 0.6°C
//  if (config->deltaT == 0xFF)  config->deltaT = 40; // 4.0°C

 #ifdef DEBUG_OUTPUT
  hbwdebug(F("Conf, maxT1: ")); hbwdebug((int16_t)config->maxT1); hbwdebug(F(" minT2: ")); hbwdebug((int16_t)config->minT2);
  hbwdebug(F(" deltaHys: ")); hbwdebug(config->deltaHys); hbwdebug(F(" deltaT: ")); hbwdebug(config->deltaT);
  hbwdebug(F("\n"));
 #endif
};


/* set special input value for a channel, via peering event. */
void HBWDeltaTx::setInfo(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  set(device, length, data);
};

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWDeltaTx::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 2)  // temperature must have 2 bytes
  {
    currentTemperature = ((data[0] << 8) | data[1]);  // set input temp (if channel is peered, the next setInfo() will overwrite the value)
    
 #ifdef DEBUG_OUTPUT
  hbwdebug(F("set: ")); hbwdebug(currentTemperature); hbwdebug(F("\n"));
 #endif
  }
};

/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDeltaTx::get(uint8_t* data)
{
  // MSB first
  *data++ = (currentTemperature >> 8);
  *data = currentTemperature & 0xFF;

  return sizeof(data);
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDeltaT::get(uint8_t* data)
{
  /* return delta temperature and state flags */
  if (deltaT >= 0 && deltaT < 255)  // deltaT is int16_t, we only send uint8_t
  {
    *data++ = deltaT;
    stateFlags.element.dlimit = true; // within the display limit
  }
  else
  {
    *data++ = 255;
    stateFlags.element.dlimit = false;  // exceeded (positive or negative)
  }
  
  *data = stateFlags.byte;

  return sizeof(data);
};


/* standard public function - called by device main loop for every channel in sequential order */
void HBWDeltaT::loop(HBWDevice* device, uint8_t channel)
{
	//uint32_t now = millis();
  static uint8_t level[2];

  stateFlags.element.mode = calculateNewState();  // calculate new deltaT value, set channel mode (active/inactive)
  
  if (millis() - outputChangeLastTime >= MIN_CHANGE_WAIT_TIME)
  {
    if (setOutput(device, channel)) // will only set output if state is different
    {
   #ifdef DEBUG_OUTPUT
    hbwdebug(F(" newstate: ")); hbwdebug(nextState); hbwdebug("\n");
   #endif
    }
  }
  
  // feedback trigger set?
  if (!nextFeedbackDelay) return;
  if (millis() - lastFeedbackTime < nextFeedbackDelay) return;
  lastFeedbackTime = millis();  // at least last time of trying
  // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
  // we know that the level has 2 byte here (value & state)
  get(level);
  if (device->sendInfoMessage(channel, sizeof(level), level) == 1) {  // bus busy
  // try again later, but insert a small delay
    nextFeedbackDelay = 250;
  }
  else {
    nextFeedbackDelay = 0;
  }
};

/* standard public function - called by device main loop for every channel in sequential order */
void HBWDeltaTx::loop(HBWDevice* device, uint8_t channel)
{
  asm ("nop");
  //uint32_t now = millis();
  // TODO add error handling if no new temperature was set after n tries or x time?
};


/* set the output port, if different to current state */
bool HBWDeltaT::setOutput(HBWDevice* device, uint8_t channel)
{
  if (currentState == nextState)  return false; // no change - quit

  digitalWrite(pin, (!nextState ^ config->n_inverted));     // set local output
  // TODO: check if peering is needed/useful. Currently not reliable...
  //if (device->busIsIdle()) {
      // don't continue if bus is not idle. sendKeyEvent would probably fail
     //keyPressNum++;
    //device->sendKeyEvent(channel, keyPressNum, currentState);  // peering (ignore inversion... can be done in peering)
    //return false?;
  //}

  outputChangeLastTime = millis();
  currentState = nextState;
  stateFlags.element.status = currentState;

  // send info/notify message in loop()
  if(!nextFeedbackDelay && config->logging) {
    lastFeedbackTime = millis();
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
  
  return true;
}


/* returns true when both input values received and valid, false if not
* retuns the current logical state, if wait time did not pass */
bool HBWDeltaT::calculateNewState()
{
  if (config->locked)  return false;
  
  if (millis() - deltaCalcLastTime >= DELTA_CALCULATION_WAIT_TIME)
  {
    deltaCalcLastTime = millis();
    
    // check if valid temp is available
    // T1
    int16_t t1 = deltaT1->currentTemperature;
    if (t1 <= ERROR_TEMP || t1 > config->maxT1) {
      nextState = OFF;
      return false;
    }
    // T2
    int16_t t2 = deltaT2->currentTemperature;
    if (t2 <= ERROR_TEMP || t2 < config->minT2) {
      nextState = OFF;
      return false;
    }
    
    deltaT = (t2 - t1) / 10;  // don't consider hundredth
    
    if ((int16_t)(deltaT >= config->deltaT)) {
      if ((int16_t)(deltaT - config->deltaHys) >= config->deltaT)
        nextState = ON;
    }
    else {
      if ((int16_t)(deltaT + config->deltaHys) <= config->deltaT)
        nextState = OFF;
    }
    return true;
  }
  return stateFlags.element.mode; // retun the state currently set - no change
}
