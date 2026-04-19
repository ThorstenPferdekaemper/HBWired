/*
 * HBWSPktS.cpp
 *
 * Created on: 01.07.2023
 * loetmeister.de
 * 
 */
 
#include "HBWSPktS.h"



HBWSPktS::HBWSPktS(uint8_t* _pin, hbw_config_dim_spkts* _config, HBWDeltaTx* _temp1, volatile unsigned char* _SPktS_currentValue) :
  pin(_pin),
  SPktSValue(_SPktS_currentValue),
  config(_config),
  temp1(_temp1)
{
  currentValue = 0;
  lastSetTime = 0;
  clearFeedback();
  stateFlags.byte = 0;
  initDone = false;
};


// channel specific settings or defaults
void HBWSPktS::afterReadConfig()
{
  if (initDone == false)
  {
  // All off on init
    *SPktSValue = 0;
    digitalWrite(*pin, LOW);
    pinMode(*pin, OUTPUT);
    initDone = true;
  }

  if (config->auto_off == 0xFF)  config->auto_off = 70;  // 70 seconds
  if (config->max_temp == 0xFF)  config->max_temp = 60;
  if (config->max_output == 0xF)  config->max_output = 9;  // 9+1 *10 = 100%

 #ifdef DEBUG_OUTPUT
  hbwdebug(F("Conf, auto_off: ")); hbwdebug(config->auto_off);
  hbwdebug(F(" max_temp: ")); hbwdebug(config->max_temp);
  hbwdebug(F(" max_out: ")); hbwdebug((uint8_t)(config->max_output+1) *10);
  hbwdebug(F("%\n"));
 #endif
};

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
void HBWSPktS::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  // data value is based on 100% *2 (200 == 100%)
  if (*data <= 200 && config->max_output != 0)
  {
    uint8_t maxOutput = (config->max_output +1) *20;
    uint8_t newValue = *data > maxOutput ? maxOutput : *data;
    *SPktSValue = newValue / (200 / WELLENPAKETSCHRITTE);
    currentValue = *SPktSValue *(200 / WELLENPAKETSCHRITTE);  // recalculate current level with actual stepping (possible rounding error)
    lastSetTime = millis();
    if (*SPktSValue > 0) {
      stateFlags.byte = 0;    // reset all flags
      stateFlags.state.working = true;
    }
  }

  if (config->max_output == 0)	// chan disabled
  {
    *SPktSValue = 0;
  }
 #ifdef DEBUG_OUTPUT
  hbwdebug(F("set HBWSPktS, Val: ")); hbwdebug((uint8_t)*SPktSValue);
  hbwdebug(F("\n"));
 #endif
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWSPktS::get(uint8_t* data)
{
  *data++ = currentValue;
  *data = stateFlags.byte;
  
  return 2;
};


/* standard public function - called by device main loop for every channel in sequential order */
void HBWSPktS::loop(HBWDevice* device, uint8_t channel)
{
  // TODO: add delay here? Check temp & timeout every 1 second...?

  if (*SPktSValue)
  {
    if (config->auto_off && (millis() - lastSetTime >= (uint32_t)(config->auto_off) *1000))  // timeout
    {
      *SPktSValue = 0;
      currentValue = 0;
      stateFlags.byte = 0;
      stateFlags.state.timeout = true;
      // set trigger to send info/notify message in loop()
      setFeedback(device, config->logging);
    }
    
    else if (config->max_temp && (temp1->currentTemperature > (int16_t)(config->max_temp) *100 || temp1->currentTemperature < 1))  // max or invalid temp
    {
      *SPktSValue = 0;
      currentValue = 0;
      stateFlags.state.working = false;
      stateFlags.state.tMax = true;
      // set trigger to send info/notify message in loop()
      setFeedback(device, config->logging);
    }
  }
  if (config->max_output == 0)    // disabled
  {
    stateFlags.byte = 7;
  }

  // feedback trigger set?
  checkFeedback(device, channel);
};

