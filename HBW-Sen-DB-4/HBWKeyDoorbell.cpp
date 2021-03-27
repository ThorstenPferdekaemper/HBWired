/*
 * HBWKeyDoorbell.cpp
 *
 * Created (www.loetmeister.de): 25.01.2020
 */

#include "HBWKeyDoorbell.h"

// Class HBWKeyDoorbell
HBWKeyDoorbell::HBWKeyDoorbell(uint8_t _pin, hbw_config_key_doorbell* _config, boolean _activeHigh)
{
  keyPressedMillis = 0;
  keyPressNum = 0;
  repeatCounter = 0;
  pin = _pin;
  config = _config;
  activeHigh = _activeHigh;
}


void HBWKeyDoorbell::afterReadConfig()
{
  if(config->long_press_time == 0xFF) config->long_press_time = 10;
  if (config->pullup && !activeHigh)  pinMode(pin, INPUT_PULLUP);
  else  pinMode(pin, INPUT);

  if (config->suppress_num == 255) config->suppress_num = 4;
  if (config->suppress_time == 255) config->suppress_time = 30;  // 3.0 seconds

#ifdef DEBUG_OUTPUT
  hbwdebug(F("cfg DBPin:"));
  hbwdebug(pin);
  hbwdebug(F(" repeat_long_p:"));
  hbwdebug(config->repeat_long_press);
  hbwdebug(F(" pullup:"));
  hbwdebug(config->pullup);
  hbwdebug(F("\n"));
#endif
};


void HBWKeyDoorbell::loop(HBWDevice* device, uint8_t channel)
{
  if (!config->n_input_locked)  return;  // skip locked channels
  
  uint32_t now = millis();
  if (now == 0) now = 1;  // do not allow time=0 for the below code // AKA  "der Teufel ist ein Eichhoernchen"
  
  buttonState = activeHigh ^ ((digitalRead(pin) ^ !config->n_inverted));
  
  if (now - lastKeyPressedMillis >= (uint32_t)config->suppress_time *100) {
    lastKeyPressedMillis = now;
    repeatCounter = 0;  // reset repeat counter when suppress time has passed
  }

 // sends short KeyEvent on short press and (repeated) long KeyEvent on long press
  if (buttonState) {
    // d.h. Taste nicht gedrueckt
    // "Taste war auch vorher nicht gedrueckt" kann ignoriert werden
      // Taste war vorher gedrueckt?
    if (keyPressedMillis) {
      // entprellen, nur senden, wenn laenger als KEY_DEBOUNCE_TIME gedrueckt
      // aber noch kein "long" gesendet
      if (now - keyPressedMillis >= KEY_DEBOUNCE_TIME && !lastSentLong)
      {
        if (repeatCounter == 0)
        {
          keyPressNum++;
          if (device->sendKeyEvent(channel, keyPressNum, false) != HBWDevice::BUS_BUSY)
          {
            repeatCounter = config->suppress_num;
            //TODO: play buzzer feedback?
            //playBuzzer = true;
          }
        }
        else {
          repeatCounter--;
        }
      }
      keyPressedMillis = 0;
    }
  }
  else {
    // Taste gedrueckt
    // Taste war vorher schon gedrueckt
    if (keyPressedMillis) {
      // muessen wir ein "long" senden?
      if (lastSentLong) {   // schon ein LONG gesendet
        if (now - lastSentLong >= (uint32_t)config->suppress_time *100 && config->repeat_long_press)
        {
          // alle suppress_time wiederholen, wenn repeat_long_press gesetzt
          // keyPressNum nicht erhoehen
          device->sendKeyEvent(channel, keyPressNum, true);  // long press
          lastSentLong = now;
        }
      }
      else if (now - keyPressedMillis >= long(config->long_press_time) * 100) {
        if (repeatCounter == 0) {
          // erstes LONG
          keyPressNum++;
          if (device->sendKeyEvent(channel, keyPressNum, true) != HBWDevice::BUS_BUSY)  // long press
          {
            lastSentLong = now;
            repeatCounter = config->suppress_num;
            ///TODO: play buzzer feedback?
            //playBuzzer = true;
          }
        }
      }
    }
    else {
      // Taste war vorher nicht gedrueckt
      keyPressedMillis = now;
      lastSentLong = 0;
    }
  }
};
