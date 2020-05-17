/*
 * HBWAnalogIn.cpp
 * 
 * analog input channel, max. 16 bit reading
 * 
 * Updated: 17.05.2020
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
  lastSendValue = 0;
  lastSentTime = 0;
  analogRead(pin);
};


// channel specific settings or defaults
void HBWAnalogIn::afterReadConfig() {
  if (config->update_interval == 0xFF)  config->update_interval = DEFAULT_UPDATE_INTERVAL/10;
  if (config->send_delta_value == 0xFF) config->send_delta_value = 10;  // delta as value/10 (consider factor in XML!)
  if (config->send_max_interval == 0xFFFF) config->send_max_interval = DEFAULT_UPDATE_INTERVAL *2;  // not faster than update interval
  if (config->send_min_interval == 0xFF) config->send_min_interval = 30 /10;  // 30 seconds (send_min_interval has 10 seconds stepping)
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWAnalogIn::get(uint8_t* data) {
  
  // MSB first
  *data++ = (currentValue >> 8);
  *data = currentValue & 0xFF;
  return 2;
};


/* standard public function - called by main loop for every channel in sequential order */
void HBWAnalogIn::loop(HBWDevice* device, uint8_t channel) {
  
  if (config->update_interval == 0) {   // skip disabled channels
    currentValue = 0;
    lastSendValue = 0;
    return;
  }
  
  unsigned long now = millis();
  
  // check if some values have to be send - do not send before min interval
  if (config->send_min_interval && now - lastSentTime <= (uint32_t)config->send_min_interval * 10000)  return;
  if ((config->send_max_interval && now - lastSentTime >= (uint32_t)config->send_max_interval * 1000) ||
      (config->send_delta_value && abs( currentValue - lastSendValue ) >= (unsigned int)(config->send_delta_value) * 10)) {
    // send new value
    //get(level); - using currentValue instead...
    if (device->sendInfoMessage(channel, sizeof(currentValue), (uint8_t*) &currentValue) != HBWDevice::BUS_BUSY) {
      lastSendValue = currentValue;   // store last value only on success
    }
    lastSentTime = now;   // if send failed, next try will be on send_max_interval or send_min_interval in case the value changed (send_delta_value)

#ifdef DEBUG_OUTPUT
  hbwdebug(F("adc-ch: "));  hbwdebug(channel);
  hbwdebug(F(" sent: "));  hbwdebug(lastSendValue); lastSendValue == currentValue ? hbwdebug(F(" SUCCESS!\n")) : hbwdebug(F(" FAILED!\n"));
#endif
  }
  
  if (now - lastActionTime < ((uint32_t)nextActionDelay *10000)) return; // quit if wait time not yet passed
    
  nextActionDelay = SAMPLE_INTERVAL;
  #define MAX_SAMPLES 3    // update "buffer" array definition, when changing this
  static uint16_t buffer[MAX_SAMPLES] = {0, 0, 0};
  static uint8_t nextIndex = 0;
  
  buffer[nextIndex++] = analogRead(pin);
  lastActionTime = now;
  
  if (nextIndex >= MAX_SAMPLES) {
    nextIndex = 0;
    uint32_t sum = 0;
    uint8_t i = MAX_SAMPLES;
    do {
       sum += buffer[--i];
    }
    while (i);
    
    currentValue = sum / MAX_SAMPLES;
    // "sleep" until next update
    nextActionDelay = config->update_interval;
    
#ifdef DEBUG_OUTPUT
  hbwdebug(F("adc-ch:"));  hbwdebug(channel);  hbwdebug(F(" measured:"));  hbwdebug(currentValue);  hbwdebug(F("\n"));
#endif
  }
};

