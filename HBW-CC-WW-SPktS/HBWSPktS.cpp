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

void setup_timer()  // call from main setup()
{
  cli();//stop interrupts

  //set timer1 interrupt at 50.08 ~50Hz (with 16MHz system speed / OSC)
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 312;// = (16*10^6) / (50*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts
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
  if (*data <= 200 && config->max_output != 0)
  {
    //uint8_t newValue = *data;// /2;
    uint8_t maxOutput = (config->max_output +1) *20;
    // if (newValue > maxOutput)
    // {
    //   newValue = maxOutput;
    // }
    uint8_t newValue = *data > maxOutput ? maxOutput : *data;
    *SPktSValue = newValue / (200 / WellenpaketSchritte);
    currentValue = *SPktSValue *(200 / WellenpaketSchritte);  // recalculate current level with actual stepping
    lastSetTime = millis();
    stateFlags.byte = 0;    // reset all flags
    stateFlags.state.working = true;
    // stateFlags.state.tMax = false;
  }

  if (config->max_output == 0)	// chan disabled
  {
    *SPktSValue = 0;
    stateFlags.byte = 0;
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
  // TODO: add delay here? new temperatue will usually not come faster than evevery 10 seconds...

  if (*SPktSValue)
  {
    if ( config->max_output == 0 || (config->auto_off && (millis() - lastSetTime >= (uint32_t)(config->auto_off) *1000)))  // disabled or timeout
    {
      *SPktSValue = 0;
      currentValue = 0;
      stateFlags.byte = 0;
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
  
  // if (*SPktSValue && millis() - lastSetTime >= (uint32_t)(config->auto_off) *1000)
  // {
  //   SPktSValue = 0;
  //   stateFlags.state.working = false;
  // }

  // if (*SPktSValue && config->max_temp && (temp1->currentTemperature > (int16_t)(config->max_temp) *100 || temp1->currentTemperature < 0))
  // {
  //   *SPktSValue = 0;
  //   stateFlags.state.working = false;
  //   stateFlags.state.tMax = true;
  // }

  // feedback trigger set?
  checkFeedback(device, channel);
};

