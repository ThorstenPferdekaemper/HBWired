/*
 * HBWKeyVirtual.cpp
 *
 * Updated (www.loetmeister.de): 9.08.2020
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
  updateDone = false;
}


// sends a short KeyEvent for mappedChan state not 0 and long KeyEvent for state equals 0
void HBWKeyVirtual::loop(HBWDevice* device, uint8_t channel) {
  
  if (config->input_locked)  return;   // locked by default

  if (millis() - keyPressedMillis < POLLING_WAIT_TIME)  return;

  uint8_t value;
  device->get(mappedChan, &value);  // check length? switch or dimmer use only 1 byte

  value = (value == 0) ? false : true;
  sendLong = (value == config->n_inverted) ? false : true;

  //sendLong = value ? false : true;
  
   if (!config->n_update_on_start && !updateDone) {
       lastSentLong = !sendLong;  // force to send key event on device start (disabled by defaullt)
       updateDone = true;
   }
  
  if (lastSentLong != sendLong){
    // send short key event immediately, delay long key event (off delay)
    if (millis() - keyPressedMillis > OFF_DELAY_TIME || !sendLong) {
        
  #ifdef DEBUG_OUTPUT
    hbwdebug(F("VKey_ch:"));
    hbwdebug(mappedChan);
    if (sendLong) hbwdebug(F(" long")); else  hbwdebug(F(" short"));
    hbwdebug(F("\n"));
  #endif

      if (device->sendKeyEvent(channel, keyPressNum, sendLong) != HBWDevice::BUS_BUSY) {
        keyPressNum++;
        lastSentLong = sendLong;
      }
    }
  }
  else {
    keyPressedMillis = millis();
  }
};
