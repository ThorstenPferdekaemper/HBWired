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
  outputChangeLastTime = 0;
  deltaCalcLastTime = 0;
  clearFeedback();
  stateFlags.byte = 0;
  keyPressNum = 0;
  initDone = false;
  evenCycle = false;
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
    initDone = true;
  }
  else {
  // Do not reset outputs on config change (EEPROM re-reads), but update its state
    digitalWrite(pin, !currentState ^ config->n_inverted);
  }
  
  // channel is disabled by default. Valid values must be set when being enabled...
//  if (config->deltaHys == 31)  config->deltaHys = 6; // 0.6°C
//  if (config->deltaT == 0xFF)  config->deltaT = 40; // 4.0°C

 #ifdef DEBUG_OUTPUT
  hbwdebug(F("Conf, maxT1: ")); hbwdebug((int16_t)config->maxT1); hbwdebug(F(" minT2: ")); hbwdebug((int16_t)config->minT2);
  hbwdebug(F(" deltaHys: ")); hbwdebug(config->deltaHys);
  hbwdebug(F(" Hys@maxT1? ")); hbwdebug(!config->n_enableHysMaxT1); hbwdebug(F(" @minT2? ")); hbwdebug(!config->n_enableHysMinT2);
  hbwdebug(F(" deltaT: ")); hbwdebug(config->deltaT); hbwdebug(F(" Hys@OFF? ")); hbwdebug(!config->n_enableHysOFF);
  hbwdebug(F(" change_wait: ")); hbwdebug(config->output_change_wait_time);
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


void HBWDeltaT::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  // allow to set output manually, only when 'mode' is idle/inactive (i.e. no T1 & T2 temperature received)
  // output will change not faster than "output_change_wait_time"
  if (!stateFlags.element.mode)
  {
    nextState = *data == 0 ? OFF : ON;
  }
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
  stateFlags.element.mode = calculateNewState();  // calculate new deltaT value, set channel mode (active/inactive)
  
  if (millis() - outputChangeLastTime >= ((uint16_t)(config->output_change_wait_time +1) *5000)/2)
  {
    if (setOutput(device, channel)) // will only set output if state is different
    {
   #ifdef DEBUG_OUTPUT
    hbwdebug(F("newstate: ")); hbwdebug(nextState);
   #endif
    }
 #ifdef DEBUG_OUTPUT
  hbwdebug(F(" ch: ")); hbwdebug(channel);
  hbwdebug(F(" deltaT: ")); hbwdebug(deltaT);
  hbwdebug(F("\n"));
 #endif
  }
  
  // feedback trigger set?
  checkFeedback(device, channel);
};

/* standard public function - called by device main loop for every channel in sequential order */
// void HBWDeltaTx::loop(HBWDevice* device, uint8_t channel)
// {
  // //uint32_t now = millis();
  // // TODO add error handling if no new temperature was set after n tries or x time?
// };


/* set the output port, if different to current state */
bool HBWDeltaT::setOutput(HBWDevice* device, uint8_t channel)
{
  outputChangeLastTime = millis();
  
  if (evenCycle) {
    evenCycle = false;
  }
  else {
    evenCycle = true;
    if (nextState == ON && !config->n_output_change_pulse) {
      digitalWrite(pin, (!OFF ^ config->n_inverted));
    }
    return false;
  }

  digitalWrite(pin, (!nextState ^ config->n_inverted));     // set local output always

  if (currentState == nextState)  return false; // no change - quit

 #ifdef DT_ENABLE_PEERING
  // allow peering with external switches
  if (device->sendKeyEvent(channel, keyPressNum, !nextState) != HBWDevice::BUS_BUSY) {
    keyPressNum++;
    currentState = nextState; // retry, as long as the bus is busy (retry interval: 'output_change_wait_time')
  }
 #else
  currentState = nextState;
 #endif  // DT_ENABLE_PEERING
  
  stateFlags.element.status = currentState;

  // set trigger to send info/notify message in loop()
  setFeedback(device, config->logging);
  
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
        
    int16_t t1 = deltaT1->currentTemperature;
    int16_t t2 = deltaT2->currentTemperature;
    
    // check if valid temps are available
    if (t1 == DEFAULT_TEMP || t2 == DEFAULT_TEMP) {  // don't overwrite 'nextState' (keep manual state) if no temp was ever received
      return false;
    }
    if (t1 <= ERROR_TEMP || t2 <= ERROR_TEMP) {  // set output to error state, when sensor got lost (error_temperature is received)
      nextState = config->error_state ? OFF : ON;
      return false;
    }

    deltaT = (t2 - t1) / 10;  // don't consider hundredth

    // check for limits
    if (t1 >= config->maxT1 || t2 < config->minT2) {
      nextState = OFF;
      return true;
    }
    
    // check delta temperature
    if ((int16_t)(deltaT > config->deltaT))
    {
      if ((int16_t)(deltaT - config->deltaHys) >= config->deltaT) {
        nextState = ON;
        if (currentState == OFF) {
          // delay ON transition for deltaHys value, if enabled
          if (!config->n_enableHysMaxT1 && (int16_t)(config->maxT1 - t1) < (config->deltaHys *10) && (int16_t)(config->maxT1 - t1) >= 0)
            nextState = OFF;
          if (!config->n_enableHysMinT2 && (int16_t)(t2 - config->minT2) < (config->deltaHys *10) && (int16_t)(t2 - config->minT2) >= 0)
            nextState = OFF;
        }
      }
    }
    else {
      if ((int16_t)(deltaT + (config->deltaHys *(!config->n_enableHysOFF))) <= config->deltaT)
        nextState = OFF;
    }
    return true;
  }
  else
    return stateFlags.element.mode; // retun the state currently set - no change
}
