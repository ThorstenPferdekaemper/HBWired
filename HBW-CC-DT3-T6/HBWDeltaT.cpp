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
  setOutputLastTime = 0;
  setOutputWaitTime = DELTAT_CALCULATION_WAIT_TIME *4;  // start delay
  deltaCalcLastTime = 0;
  clearFeedback();
  stateFlags.byte = 0;
  keyPressNum = 0;
  initDone = false;
  forceOutputChange = false;
  sendKeyEventFailCounter = SEND_KEY_EVENT_MAX_RETRY;
};


HBWDeltaTx::HBWDeltaTx(hbw_config_DeltaTx* _config)
{
  config = _config;
  currentTemperature = DEFAULT_TEMP; // chan must be (t > ERROR_TEMP) to work
  tempLastTime = 0;
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
    nextState = currentState;
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
    tempLastTime = millis();
    errorCounter = config->max_tries;  // reset error counter
    
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

  return 2;  // 2 byte
};


void HBWDeltaT::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  // Forcefully set output. If 'mode' is active, the output state will fallback to calculated state after "output_change_wait_time"
  // to keep output in ON / OFF state, repeat set FORCE_OUTPUT_ON / FORCE_OUTPUT_OFF faster than "output_change_wait_time"
  // 203 - force OFF
  // 205 - force ON
  if (*data == 203 || *data == 205)
  {
    nextState = (*data == 203) ? OFF : ON;
    forceOutputChange = true;
    setOutputWaitTime = 0;  // override output_change_wait_time
  }
  // allow to set output manually, only when 'mode' is idle/inactive (i.e. no T1 or T2 temperature received)
  // output will change not faster than "output_change_wait_time"
  else if (*data >= 200 && !stateFlags.element.mode)
  {
    nextState = (*data == 0) ? OFF : ON;
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
  else {
    *data++ = 255;
    stateFlags.element.dlimit = false;  // exceeded (positive or negative)
  }
  
  *data = stateFlags.byte;

  return 2;  // 1 byte value and 1 byte state
};


/* standard public function - called by device main loop for every channel in sequential order */
void HBWDeltaT::loop(HBWDevice* device, uint8_t channel)
{
  if (config->locked)
  {
    forceOutputChange = true; // skip calculateNewState()
    // force output to error_state when channel is locked
    nextState = config->n_error_state ? OFF : ON;
    if (currentState != !config->n_error_state)
    {
      setOutputWaitTime = 0;  // don't wait
    }
  }

  // do not allow new values / state when "inhibit" is enabled
  // also skip when output state is manually forced (forceOutputChange)
  stateFlags.element.mode = calculateNewState(forceOutputChange || getLock());  // calculate new deltaT value, set channel mode (active/inactive) and nextState

  if (setOutput(device, channel)) // will only set output if state is different
  {
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("ch: ")); hbwdebug(channel);
  hbwdebug(F(" deltaT: ")); hbwdebug(deltaT);
  hbwdebug(F(" newstate: ")); hbwdebug(nextState);
  hbwdebug(F("\n"));
  #endif
  }
  
  // feedback trigger set?
  checkFeedback(device, channel);
};

/* standard public function - called by device main loop for every channel in sequential order */
void HBWDeltaTx::loop(HBWDevice* device, uint8_t channel)
{
  unsigned long now = millis();

  // error handling if no new temperature was set after x time * n times
  // check only if we had a valid temperature, but not faster than min RECEIVE_MAX_INTERVAL (5 seconds)
  if (currentTemperature > ERROR_TEMP && now - (tempLastTime >= 5000))
  {
    if (config->receive_max_interval <= 3600 && config->receive_max_interval >= 5)
    {
      // RECEIVE_MAX_INTERVAL is enabled, wait for configured timeout
      if (now - tempLastTime >= (unsigned long)config->receive_max_interval *1000)
      {
        if (errorCounter == 0) {
          currentTemperature = ERROR_TEMP;
        }
        else {
          errorCounter--;
          tempLastTime = now;  // wait another inverval
        }
      }
    }
    else {
      // RECEIVE_MAX_INTERVAL is disabled or invalid. Still don't go into this loop faster than min RECEIVE_MAX_INTERVAL (5 seconds)
      tempLastTime = now;
    }
  }
};


/* set the output port, if different to current state */
bool HBWDeltaT::setOutput(HBWDevice* device, uint8_t channel)
{
  if ((millis() - setOutputLastTime >= setOutputWaitTime))
  {
    setOutputLastTime = millis();
    bool returnValue = false;

    digitalWrite(pin, (!nextState ^ config->n_inverted));     // always set local output

    if (currentState != nextState || !initDone)
    {
      // allow peering with external switches
      // force to send key events after device restart (initDone == false)
      if (device->sendKeyEvent(channel, keyPressNum, !nextState) != HBWDevice::BUS_BUSY) {
        keyPressNum++;
        currentState = nextState;
        setFeedback(device, config->logging);  // set trigger to send info/notify message
        returnValue = true;
      }
      else {
        if (sendKeyEventFailCounter > 0) {
          sendKeyEventFailCounter--;
          setOutputWaitTime = SEND_KEY_EVENT_RETRY_DELAY;
          return false;  // retry for SEND_KEY_EVENT_MAX_RETRY times
        }
        else {
          currentState = nextState;  // give up
          setFeedback(device, config->logging);  // set trigger to send info/notify message
          returnValue = true;
        }
      }
    }

    forceOutputChange = false;
    initDone = true;  // only once after device (re)start
    setOutputWaitTime = (uint16_t)(config->output_change_wait_time +1) *5000;
    sendKeyEventFailCounter = SEND_KEY_EVENT_MAX_RETRY;
    stateFlags.element.status = currentState;
    
    return returnValue;
  }
  return false;
};


/* returns true when both input values received and valid, false if not
* retuns the current logical state, if wait time did not pass */
bool HBWDeltaT::calculateNewState(bool _skip)
{
  if (_skip) return false; // TODO: skip deltaT calucalation, but check for errors & limits (max)?

  if (millis() - deltaCalcLastTime >= DELTAT_CALCULATION_WAIT_TIME)
  {
    deltaCalcLastTime = millis();
        
    int16_t t1 = deltaT1->currentTemperature;
    int16_t t2 = deltaT2->currentTemperature;
    
    // check if valid temps are available
    if (t1 == DEFAULT_TEMP || t2 == DEFAULT_TEMP) {  // don't overwrite 'nextState' (keep manual state) if no temp was ever received
      return false;
    }
    if (t1 <= ERROR_TEMP || t2 <= ERROR_TEMP) {  // set output to error state, when sensor got lost (error_temperature is received)
      nextState = config->n_error_state ? OFF : ON;
      return false;
    }

    // calculate/update delta temperature, will also be available via channel get()
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
          if (!config->n_enableHysMaxT1 && (int16_t)(config->maxT1 - t1) < (int16_t)(config->deltaHys *10) && (int16_t)(config->maxT1 - t1) >= 0)
            nextState = OFF;
          if (!config->n_enableHysMinT2 && (int16_t)(t2 - config->minT2) < (int16_t)(config->deltaHys *10) && (int16_t)(t2 - config->minT2) >= 0)
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
};
