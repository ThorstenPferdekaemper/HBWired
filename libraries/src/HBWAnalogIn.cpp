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
//void HBWAnalogIn::afterReadConfig() {
//
//};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWAnalogIn::get(uint8_t* data) {
  
  // MSB first
  *data++ = (currentValue >> 8);
  *data = currentValue & 0xFF;
  return 2;
};


/* standard public function - called by main loop for every channel in sequential order */
void HBWAnalogIn::loop(HBWDevice* device, uint8_t channel) {
  
  if (!config->input_enabled) {   // skip disabled channels
    currentValue = 0;
    return;
  }
  
  if (millis() - lastActionTime < ((uint32_t)nextActionDelay *1000)) return; // quit if wait time not yet passed
    
  nextActionDelay = SAMPLE_INTERVAL;
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
    nextActionDelay = UPDATE_INTERVAL;	// "sleep" until next update

#ifdef DEBUG_OUTPUT
  hbwdebug(F("adc-ch:"));
  hbwdebug(channel);
  hbwdebug(F(" Val:"));
  hbwdebug(currentValue);
  hbwdebug(F("\n"));
#endif
  }
};

