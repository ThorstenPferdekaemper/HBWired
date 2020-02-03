/*
 * HBWAnalogPow.cpp
 * 
 * analog power meter input channel, using current transformers connected to ADC
 * 
 * Updated: 02.04.2019
 * www.loetmeister.de
 * 
 */

#include "HBWAnalogPow.h"

// Class HBWAnalogPow
HBWAnalogPow::HBWAnalogPow(uint8_t _pin, hbw_config_analogPow_in* _config) {
  pin = _pin;
  config = _config;
  lastActionTime = 0;
  nextActionDelay = MIN_UPDATE_INTERVAL *2; // initial dealy
  currentValue = 0;
  analogRead(pin);
};


// channel specific settings or defaults
//void HBWAnalogPow::afterReadConfig() {
//
//};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWAnalogPow::get(uint8_t* data) {
  
  // MSB first
  *data++ = (currentValue >> 8);
  *data = currentValue & 0xFF;
  return 2;
};


/* standard public function - called by main loop for every channel in sequential order */
void HBWAnalogPow::loop(HBWDevice* device, uint8_t channel) {
  
  if (config->input_disabled) {   // skip disabled channels
    currentValue = 0;
    return;
  }
  
  if (millis() - lastActionTime < ((uint32_t)nextActionDelay )) return; // quit if wait time not yet passed
  
  //TODO: make it work :)
  // perform measurement in port interrupt function triggered by mains opto = mains frequency? (consider different phases 120°, 240° phase shift? - don't care if covering full sinus wave?)
  // use global buffer and run run ADC conversion for one channel at a time, with fixed amount of samples, to cover full sin wave
  // approach 1: run all samples at one interrupt (block device for 1/50Hz) or 2: run one sample per interrupt (50Hz), but delay a bit each time, to cover full sin wave
  // disable interrupt until channel fetched result and next channel should be started?
  
  // use I²C ADC? how to buffer or run continous measurement?
  
  nextActionDelay = SAMPLE_INTERVAL;
  #define MAX_SAMPLES 6    // update "buffer" array definition, when changing this!
  static uint8_t buffer[MAX_SAMPLES] = {0, 0, 0, 0, 0, 0};
  static uint8_t nextIndex = 0;
  
  uint16_t adcReading = analogRead(pin);
  if (adcReading < CENTRE_VALUE) {
    adcReading = CENTRE_VALUE - adcReading;
  }
  else
  {
    adcReading = adcReading - CENTRE_VALUE;
  }
  buffer[nextIndex++] = (uint8_t)(adcReading >>2);//analogRead(pin);
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
    // nextActionDelay = UPDATE_INTERVAL;	// "sleep" until next update
    nextActionDelay = (config->update_interval < 1) ? MIN_UPDATE_INTERVAL : (uint16_t)(config->update_interval * 1000);	// "sleep" until next update

#ifdef DEBUG_OUTPUT
  hbwdebug(F("adc-ch:"));
  hbwdebug(channel);
  hbwdebug(F(" Val:"));
  hbwdebug(currentValue);
  hbwdebug(F("\n"));
#endif
  }
};
