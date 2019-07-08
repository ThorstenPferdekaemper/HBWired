/*
 * HBWDeltaT.cpp
 *
 * Created on: 05.05.2019
 * loetmeister.de
 * 
 */
 
#include "HBWDeltaT.h"



HBWDeltaT::HBWDeltaT(uint8_t _pin, HBWDeltaTx* _delta_t1, HBWDeltaTx* _delta_t2, hbw_config_DeltaT* _config)
{
  config = _config;
  pin = _pin;
  deltaT1 = _delta_t1;
  deltaT2 = _delta_t2;

  deltaT = 0xFF;
  outputChangeLastTime = DELTA_CALCULATION_WAIT_TIME;
  deltaCalcLastTime = 0;
  lastFeedbackTime = 0;
  stateFlags.byte = 0;
  initDone = false;
//  digitalWrite(pin, LOW);
//  pinMode(pin, OUTPUT);
};


HBWDeltaTx::HBWDeltaTx(hbw_config_DeltaTx* _config)
{
  config = _config;

  currentTemperature = DEFAULT_TEMP; // chan must be (t > ERROR_TEMP) to work
};


// channel specific settings or defaults
void HBWDeltaT::afterReadConfig()
{
  if (initDone == false) {
  // All off on init, but consider inverted setting
    digitalWrite(pin, config->n_inverted ? LOW : HIGH);   // 0=inverted, 1=not inverted (device reset will set to 1!)
    pinMode(pin,OUTPUT);
    currentState = OFF;
    nextState = currentState;  //TODO: how to reset peered actors???
  }
  else {
  // Do not reset outputs on config change (EEPROM re-reads), but update its state
    digitalWrite(pin, !currentState ^ config->n_inverted);
  }
  initDone = true;
  
  // channel is disabled by default. Valid values must be set when enabled...
//  if (config->deltaHys == 31)  config->deltaHys = 6; // 0.6°C
//  if (config->deltaT == 0xFF)  config->deltaT = 40; // 4.0°C

  hbwdebug(F("Conf, maxT1: ")); hbwdebug((int16_t)config->maxT1); hbwdebug(F(" minT2: ")); hbwdebug((int16_t)config->minT2);
  hbwdebug(F(" deltaHys: ")); hbwdebug(config->deltaHys); hbwdebug(F(" deltaT: ")); hbwdebug(config->deltaT);
  hbwdebug(F("\n"));
};


/* set special input value for a channel, via peering event. */
void HBWDeltaTx::setInfo(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  set(device, length, data);
  
//  #ifdef DEBUG_OUTPUT
//  hbwdebug(F("set: ")); hbwdebug(currentTemperature); hbwdebug(F("\n"));
//  #endif
};

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWDeltaTx::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (length == 2)  // desired_temp has 2 bytes
  {
    currentTemperature = ((data[0] << 8) | data[1]);  // set input temp
    
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
  if (deltaT > 254 || deltaT < 0)
  {
    *data++ = 255;
    stateFlags.element.dlimit = true;
  }
  else
  {
    *data++ = deltaT;//lastDeltaT;
    stateFlags.element.dlimit = false;
  }
  
  *data = stateFlags.byte;

  return sizeof(data);
};


/* standard public function - called by device main loop for every channel in sequential order */
void HBWDeltaT::loop(HBWDevice* device, uint8_t channel)
{
	uint32_t now = millis();
  static uint8_t level[2];

  stateFlags.element.active = calculateNewState();
  
  if (now - outputChangeLastTime >= MIN_CHANGE_WAIT_TIME) // TODO: use DELTA_CALCULATION_WAIT_TIME * 3 ?
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
  if (now - lastFeedbackTime < nextFeedbackDelay) return;
  lastFeedbackTime = now;  // at least last time of trying
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


bool HBWDeltaT::setOutput(HBWDevice* device, uint8_t channel)
{
  if (currentState != nextState)
  {
    currentState = nextState;
    //nextState = nextState ^ (LOW ^ config->n_inverted);

    keyPressNum++;
    digitalWrite(pin, (!nextState ^ config->n_inverted));     // set local output
    device->sendKeyEvent(channel, keyPressNum, currentState);  // peering (ignore inversion... can be done in peering)

    outputChangeLastTime = millis();
    stateFlags.element.status = currentState;

    // send info/notify message in loop()
    if(!nextFeedbackDelay && config->logging) {
      lastFeedbackTime = millis();
      nextFeedbackDelay = device->getLoggingTime() * 100;
    }
    return true;
  }
  return false;
}


bool HBWDeltaT::calculateNewState()
{
  if (config->locked) return false;
  
  uint32_t now = millis();

  if (now - deltaCalcLastTime >= DELTA_CALCULATION_WAIT_TIME)
  {
    deltaCalcLastTime = now;
    
    // check if valid temp is available
    int16_t t1 = deltaT1->currentTemperature;
    if (t1 <= ERROR_TEMP || t1 > config->maxT1) {
      nextState = OFF;
      return false;
    }

    int16_t t2 = deltaT2->currentTemperature;
    if (t2 <= ERROR_TEMP || t2 < config->minT2) {
      nextState = OFF;
      return false;
    }
    

    deltaT = (t2 - t1) / 10;  // don't consider hundredth
    if (deltaT >= config->deltaT) {
      if ((int16_t)(deltaT - config->deltaHys) >= config->deltaT) nextState = ON;
    }
    else  {
      if ((int16_t)(deltaT + config->deltaHys) <= config->deltaT)  nextState = OFF;
    }
    
//  #ifdef DEBUG_OUTPUT
//  hbwdebug(F(" deltaT: ")); hbwdebug(deltaT); hbwdebug(F("d°C"));
//  hbwdebug(F(" nextState: ")); hbwdebug(nextState); hbwdebug(F("\n"));
//  #endif
  }
  return true;
}
