/*
 * HBWSenEP.cpp
 *
 *  Created on: 30.04.2019
 *      Author: loetmeister.de
 *      Vorlage: Markus, Thorsten
 */


#include "HBWSenEP.h"


// constructor
HBWSenEP::HBWSenEP(uint8_t _pin, hbw_config_sen_ep* _config, boolean _activeHigh) {
  pin = _pin;
  config = _config;
  activeHigh = _activeHigh;
  
  currentCount = 0;
  lastSentCount = 0;
  lastSentTime = 0;
  lastPortReadTime = 0;
  
  if (activeHigh) pinMode(pin, INPUT);
  else pinMode(pin, INPUT_PULLUP); // pullup only for activeLow
}

// constructor when using INTERRUPT based counter
HBWSenEP::HBWSenEP(volatile uint16_t* _counter, hbw_config_sen_ep* _config, boolean _activeHigh) {
  counter = _counter;
  config = _config;
  activeHigh = _activeHigh;
  
  pin = NOT_A_PIN;
  currentCount = 0;
  lastSentCount = 0;
  lastSentTime = 0;
  lastPortReadTime = 0;
}


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWSenEP::afterReadConfig() {
  if(config->send_delta_count == 0xFFFF) config->send_delta_count = 1;
  if(config->send_min_interval == 0xFFFF) config->send_min_interval = 10;
  if(config->send_max_interval == 0xFFFF) config->send_max_interval = 600; // 10 minutes

 #ifdef DEBUG_OUTPUT
  hbwdebug(F("Pin: ")); hbwdebug(pin); hbwdebug(F(" enabled: ")); hbwdebug(config->enabled);hbwdebug(F("\n"));
  // hbwdebug(F(" poll_time: ")); hbwdebug((uint8_t)config->polling_time *10); hbwdebug(F("ms\n"));
 #endif
}


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWSenEP::get(uint8_t* data) {
  
  // MSB first
  *data++ = (currentCount >> 8);
  *data = currentCount & 0xFF;
  return 2;
}


void HBWSenEP::loop(HBWDevice* device, uint8_t channel) {

  if (!config->enabled) {  // skip disabled channels, reset counter
    currentCount = 0;
    lastSentCount = 0;
    return;
  }
  
  uint32_t now = millis();
  
 //skip this if ISR is used
  if (pin != NOT_A_PIN) {
    currentPortState = readInput(pin);
    
    if (currentPortState == LOW)
    {
      if (!lastPortReadTime)  // not pressed yet
      {
        lastPortReadTime = (now == 0) ? 1 : now;
        portStateChange = true;
      }
      else if (now - lastPortReadTime > DEBOUNCE_DELAY && portStateChange) {
      // only count transition from high to low - once
        portStateChange = false;
        currentCount++;
       #ifdef DEBUG_OUTPUT
        hbwdebug(F("Pin: ")); hbwdebug(pin); hbwdebug(F(" COUNT:")); hbwdebug(currentCount);hbwdebug(F("\n"));
       #endif
      }
    }
    else {
      portStateChange = true;
      lastPortReadTime = 0;
    }
  }
  else
  {
    currentCount = *counter;  // read current counter value - updated by interrupt function in the background
  }

  // check if some data needed to be send
  // do not send before min interval
  if (config->send_min_interval && now - lastSentTime < (uint32_t)config->send_min_interval * 1000)  return;
  if ((config->send_max_interval && now - lastSentTime >= (uint32_t)config->send_max_interval * 1000) ||
      (config->send_delta_count && (currentCount - lastSentCount ) >= config->send_delta_count)) {
    // send
    uint8_t level[2];
    get(level);
    // if bus is busy, then we try again in the next round
    if (device->sendInfoMessage(channel, 2, level) != HBWDevice::BUS_BUSY) {    // level has always 2 byte here
      lastSentCount = currentCount;
      lastSentTime = now;
    
     #ifdef DEBUG_OUTPUT
      hbwdebug(F("Ch: ")); hbwdebug(channel); hbwdebug(F(" send: ")); hbwdebug(lastSentCount); hbwdebug(F("\n"));
     #endif
    }
  }
}
