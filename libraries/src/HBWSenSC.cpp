/*
 * HBWSenSC.cpp
 * 
 * sensor/shutter contact
 * Query to this type of channel will return "contact_closed" or "contact_open" (boolean)
 * No peering. I-Message/notify on status change can be enabled by NOTIFY: on/off
 * 
 * Updated: 09.08.2020
 * www.loetmeister.de
 * 
 */

#include "HBWSenSC.h"

// Class HBWSenSC
HBWSenSC::HBWSenSC(uint8_t _pin, hbw_config_senSC* _config, boolean _activeHigh) {
  pin = _pin;
  config = _config;
  activeHigh = _activeHigh;
  keyPressedMillis = 0;
  clearFeedback();
  initDone = false;
  if (activeHigh) pinMode(pin, INPUT);
  else pinMode(pin, INPUT_PULLUP); // pullup only for activeLow
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWSenSC::get(uint8_t* data) {
  
  (*data) = currentValue ? 200 : 0;
  
  return 1;
};


/* standard public function - called by main loop for every channel in sequential order */
void HBWSenSC::loop(HBWDevice* device, uint8_t channel) {
  
  //avoid notify messages on device start
  if (initDone == false) {
    currentValue = readScInput();
    initDone = true;
    return;
  }
  
  if (!config->n_input_locked) return;   // channel locked?
  
  uint32_t now = millis();
  
  boolean buttonState = readScInput();
  
  if (buttonState != currentValue) {
    
    if (!keyPressedMillis) {
      // Taste war vorher nicht gedrueckt
      keyPressedMillis = now ? now : 1;
    }
    else if (now - keyPressedMillis >= DEBOUNCE_TIME) {
      currentValue = buttonState;
      keyPressedMillis = 0;
      
      clearFeedback();   // rapid state changes would reset the timer and not flood the bus with messages
      setFeedback(device, !config->notify_disabled, DEBOUNCE_TIME *10);
      
  #ifdef DEBUG_OUTPUT
    hbwdebug(F("sc-ch:"));
    hbwdebug(channel);
    hbwdebug(F(" val:"));
    hbwdebug(currentValue);
    hbwdebug(F("\n"));
  #endif
    }
  }
  else {
    keyPressedMillis = 0;
  }
  
  // feedback trigger set?
  checkFeedback(device, channel);
};

