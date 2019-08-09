/*
 * HBWKeyVirtual.cpp
 *
 * Updated (www.loetmeister.de): 5.08.2019
 * - Added: input_locked, n_inverted, input_type (SWITCH, MOTIONSENSOR, DOORSENSOR)
 * - Changes require new XML layout for config!
 * 
 */

#include "HBWKeyVirtual.h"

// Class HBWKeyVirtual
HBWKeyVirtual::HBWKeyVirtual(uint8_t _mappedChan, hbw_config_key_virt* _config) :
mappedChan(_mappedChan),
config(_config)
{
  keyPressNum = 0;
  keyPressedMillis = 0;
  forceUpdate = false;  //TODO: add option to force update on start/config changes?
}


// sends a short KeyEvent on HIGH and long KeyEvent on LOW level changes
void HBWKeyVirtual::loop(HBWDevice* device, uint8_t channel) {
  
  if (config->input_locked)  return;   // locked by default

  if (millis() - keyPressedMillis < POLLING_WAIT_TIME)  return;

  uint8_t value;
  device->get(mappedChan, &value);  // check length?

//    if (value <= config->threshold) {
  if (value == 0) {
    sendLong = true ^ !config->n_inverted;
  }
  else {
     sendLong = false ^ !config->n_inverted;
  }
  
  
  if (lastSentLong != sendLong || forceUpdate) {
    // send short key event immediately, delay long key event (off delay)
    if (millis() - keyPressedMillis > OFF_DELAY_TIME || !sendLong) {
      if (!device->busIsIdle())
        return; // don't continue if bus is not idle. sendKeyEvent would probably fail
        
  #ifdef DEBUG_OUTPUT
    hbwdebug(F("VKey_ch:"));
    hbwdebug(mappedChan);
    if (sendLong) hbwdebug(F(" long")); else  hbwdebug(F(" short"));
    hbwdebug(F("\n"));
  #endif

      keyPressNum++;
      lastSentLong = sendLong;
      device->sendKeyEvent(channel, keyPressNum, sendLong);
      
      forceUpdate = false;
    }
  }
  else {
    keyPressedMillis = millis();
  }
};
