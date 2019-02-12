/*
 * HBWKeyVirtual.cpp
 *
 * Updated (www.loetmeister.de): 9.12.2018
 * - Added: input_locked, n_inverted, input_type (SWITCH, MOTIONSENSOR, DOORSENSOR)
 * - Changes require new XML layout for config!
 * 
 */

#include "HBWKeyVirtual.h"

// Class HBWKeyVirtual
HBWKeyVirtual::HBWKeyVirtual(uint8_t _mappedChan, hbw_config_key_virt* _config) {
  keyPressNum = 0;
  keyPressedMillis = 0;
  lastSentLong = !sendLong;
  mappedChan = _mappedChan;
  config = _config;
}


//void HBWKeyVirtual::afterReadConfig() {
//  if (config->threshold == 0xFF)  config->threshold = 0;
//};


// sends a short KeyEvent on HIGH and long KeyEvent on LOW level changes
void HBWKeyVirtual::loop(HBWDevice* device, uint8_t channel) {
  
  if (!config->input_locked) {   // skip locked channels

    uint8_t value;
    device->get(mappedChan, &value);

//    if (value <= config->threshold) {
    if (value == 0) {
      sendLong = false ^ config->n_inverted;
    }
    else {
      sendLong = true ^ config->n_inverted;
    }
    
    if (lastSentLong != sendLong) {
      // send short key event immediately, delay long key event (off delay)
      if (millis() - keyPressedMillis > KEY_DELAY_TIME || !sendLong) {

    #ifdef DEBUG_OUTPUT
      hbwdebug(F("VKey_ch:"));
      hbwdebug(mappedChan);
      if (sendLong) hbwdebug(F(" long")); else  hbwdebug(F(" short"));
      hbwdebug(F("\n"));
    #endif
  
        keyPressNum++;
        lastSentLong = sendLong;
        device->sendKeyEvent(channel, keyPressNum, sendLong);
      }
    }
    else {
      keyPressedMillis = millis();
    }
  }
};
