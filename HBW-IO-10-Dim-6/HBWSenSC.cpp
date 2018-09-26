/*
 * HBWSenSC.cpp
 * 
 * sensor/shutter contact 
 * 
 * Updated: 09.09.2018
 * www.loetmeister.de
 * 
 */

#include "HBWSenSC.h"

// Class HBWSenSC
HBWSenSC::HBWSenSC(uint8_t _pin, hbw_config_senSC* _config) {
  pin = _pin;
  config = _config;
  keyPressedMillis = 0;
  currentValue = false;
  pinMode(pin, INPUT);
};


// channel specific settings or defaults
void HBWSenSC::afterReadConfig() {
    
#ifdef DEBUG_OUTPUT
  hbwdebug(F("cfg SCPin:"));
  hbwdebug(pin);
  hbwdebug(F("\n"));
#endif
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWSenSC::get(uint8_t* data) {
  
  if (currentValue)
    (*data) = 200;
  else
    (*data) = 0;
  return 1;
};


/* standard public function - called by main loop for every channel in sequential order */
void HBWSenSC::loop(HBWDevice* device, uint8_t channel) {
  
  if (config->n_input_locked) {   // not locked?

    bool buttonState = (digitalRead(pin) ^ !config->n_inverted);
    
    if (buttonState != currentValue) {
      
    uint32_t now = millis();
      
    if (!keyPressedMillis) {
      // Taste war vorher nicht gedrueckt
      keyPressedMillis = now ? now : 1;
    }
    else if (now - keyPressedMillis >= DEBOUNCE_TIME) {
      currentValue = buttonState;
      keyPressedMillis = 0;
      //TODO: check if broadcast message should be send on state changes? (avoid risk to 'spam' the bus?)
        
  #ifdef DEBUG_OUTPUT
    hbwdebug(F("SC-ch:"));
    hbwdebug(channel);
    hbwdebug(F(" val:"));
    hbwdebug(currentValue);
    hbwdebug(F("\n"));
  #endif
      }
    }
  }
};

