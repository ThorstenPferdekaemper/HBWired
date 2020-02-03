/*
 * HBWAnalogIn.cpp
 * 
 * analog input channel, max. 16 bit reading
 * 
 * Updated: 29.09.2018
 * www.loetmeister.de
 * 
 */

#include "HBWAnalogIn.h"

// Class HBWAnalogIn
HBWAnalogIn::HBWAnalogIn(uint8_t _pin, hbw_config_analog_in* _config) {
  pin = _pin;
  config = _config;
  lastActionTime = 0;
  nextActionDelay = SAMPLE_INTERVAL *2; // initial dealy
  currentValue = 0;
  analogRead(pin);
};


// channel specific settings or defaults
// void HBWAnalogIn::afterReadConfig() {
  // if (config->update_interval == 0xFF || config->update_interval == 0)  config->update_interval = 30; // default 300 seconds
// };


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWAnalogIn::get(uint8_t* data) {
  
  // MSB first
  *data++ = (currentValue >> 8);
  *data = currentValue & 0xFF;
  return 2;
};


/* standard public function - called by main loop for every channel in sequential order */
void HBWAnalogIn::loop(HBWDevice* device, uint8_t channel) {
  
  if (config->input_disabled) {   // skip disabled channels
    currentValue = 0;
    return;
  }
  
  if (millis() - lastActionTime < ((uint32_t)nextActionDelay *10000)) return; // quit if wait time not yet passed
    
  nextActionDelay = SAMPLE_INTERVAL;
  #define MAX_SAMPLES 3    // update "buffer" array definition, when changing this
  static uint16_t buffer[MAX_SAMPLES] = {0, 0, 0};
  static uint8_t nextIndex = 0;
  
  buffer[nextIndex++] = analogRead(pin);
  lastActionTime = millis();
  
  if (nextIndex >= MAX_SAMPLES) {
    nextIndex = 0;
    uint32_t sum = 0;
    uint8_t i = MAX_SAMPLES;
    do {
       sum += buffer[--i];
    }
    while (i);
    
    currentValue = sum / MAX_SAMPLES;
    nextActionDelay = config->update_interval;	// "sleep" until next update
    nextActionDelay = (config->update_interval == 0) ? DEFAULT_UPDATE_INTERVAL/10 : config->update_interval;

#ifdef DEBUG_OUTPUT
  hbwdebug(F("adc-ch:"));
  hbwdebug(channel);
  hbwdebug(F(" Val:"));
  hbwdebug(currentValue);
  hbwdebug(F("\n"));
#endif
  }
};

