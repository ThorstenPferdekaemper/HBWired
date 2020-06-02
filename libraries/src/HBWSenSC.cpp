/*
 * HBWSenSC.cpp
 * 
 * sensor/shutter contact
 * Query to this type of channel will return "contact_closed" or "contact_open" (boolean)
 * No peering. I-Message/notify on status change can be enabled by NOTIFY: on/off
 * 
 * Updated: 29.09.2018
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
  nextFeedbackDelay = 0;
  currentState = false; // init flag
  if (activeHigh) pinMode(pin, INPUT);
  else pinMode(pin, INPUT_PULLUP); // pullup only for activeLow
};


// channel specific settings or defaults
void HBWSenSC::afterReadConfig() {

  //avoid notify messages on device start
  if (currentState == false) {
    currentValue = readScInput();
    currentState = true;
  }
  
#ifdef DEBUG_OUTPUT
  hbwdebug(F("cfg SCPin:"));
  hbwdebug(pin);
  hbwdebug(F(" val:"));
  hbwdebug(currentValue);
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
    boolean buttonState = readScInput();
    
    if (buttonState != currentValue) {
      uint32_t now = millis();
      
      if (!keyPressedMillis) {
        // Taste war vorher nicht gedrueckt
        keyPressedMillis = now ? now : 1;
      }
      else if (now - keyPressedMillis >= DEBOUNCE_TIME) {
        currentValue = buttonState;
        keyPressedMillis = 0;
  
        if (!config->notify_disabled) {  // notify/i-message enabled
          lastFeedbackTime = now;   // rapid state changes would reset the counter and not flood the bus with messages
          nextFeedbackDelay = DEBOUNCE_TIME +30;
        }
        
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
    if(!nextFeedbackDelay) return;
    if(millis() - lastFeedbackTime < ((uint32_t)nextFeedbackDelay *10)) return;
    lastFeedbackTime = millis();  // at least last time of trying
    // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
    // we know that the level has only 1 byte here
    static uint8_t level;
    get(&level);
    device->sendInfoMessage(channel, 1, &level);
    nextFeedbackDelay = 0;  // do not resend (sendInfoMessage perform 3 retries anyway)
  }
};

