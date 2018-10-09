/*
 * HBWSenSC.cpp
 * 
 * sensor/shutter contact
 * Query to this type channel will return "contact_closed" or "contact_open" (boolean)
 * No peering. I-Message/notify on status change can be enabled by NOTIFY: on/off
 * 
 * Updated: 29.09.2018
 * www.loetmeister.de
 * 
 */

#include "HBWSenSC.h"

// Class HBWSenSC
HBWSenSC::HBWSenSC(uint8_t _pin, hbw_config_senSC* _config) {
  pin = _pin;
  config = _config;
  keyPressedMillis = 0;
  nextFeedbackDelay = 0;
  currentState = false; // use as init flag
  
  pinMode(pin, INPUT);
};


// channel specific settings or defaults
void HBWSenSC::afterReadConfig() {

  //avoid notify messages on device start
  if (currentState == false) {
    currentValue = (digitalRead(pin) ^ !config->n_inverted);
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
  
        if (!config->notify_disabled) {  // notify/i-message enabled
          lastFeedbackTime = now;   // rapid state changes would reset the counter and not flood the bus with messages
          nextFeedbackDelay = DEBOUNCE_TIME +300;
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
    if(millis() - lastFeedbackTime < nextFeedbackDelay) return;
    lastFeedbackTime = millis();  // at least last time of trying
    // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
    // we know that the level has only 1 byte here
    uint8_t level;
    get(&level);
    device->sendInfoMessage(channel, 1, &level);
    nextFeedbackDelay = 0;  // do not resend (sendInfoMessage perform 3 retries anyway)
  }
};

