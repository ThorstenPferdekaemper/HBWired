/*
 * HBWKey.cpp
 *
 * Updated (www.loetmeister.de): 29.09.2018
 * - Added: input_locked, n_inverted, input_type (SWITCH, MOTIONSENSOR, DOORSENSOR)
 * - Changes require new XML layout for config!
 * 
 * Updated (www.loetmeister.de): 19.05.2020
 * - Added resend capability on busy bus, for MOTIONSENSOR and DOORSENSOR input_type
 */

#include "HBWKey.h"

// Class HBWKey
HBWKey::HBWKey(uint8_t _pin, hbw_config_key* _config, boolean _activeHigh) {
  keyPressedMillis = 0;
  keyPressNum = 0;
  pin = _pin;
  config = _config;
  activeHigh = _activeHigh;
}


void HBWKey::afterReadConfig(){
  if(config->long_press_time == 0xFF) config->long_press_time = 10;
  if (config->pullup && !activeHigh)  pinMode(pin, INPUT_PULLUP);
  else  pinMode(pin, INPUT);

#ifdef DEBUG_OUTPUT
  hbwdebug(F("cfg KeyPin:"));
  hbwdebug(pin);
  hbwdebug(F(" type:"));
  hbwdebug(config->input_type);
  hbwdebug(F(" pullup:"));
  hbwdebug(config->pullup);
  hbwdebug(F("\n"));
#endif
};


/* standard public function - returns length of data array. Data array contains current channel reading */
#ifdef ENABLE_SENSOR_STATE
uint8_t HBWKey::get(uint8_t* data) {
  
  (*data) = keyPressedMillis ? 200 : 0;  // use keyPressedMillis, that already reflect activeHigh and inverted option
  (*data++) = 0;  // state flags not used, but included in INFO_LEVEL frame type
  return 2;
};
#endif


void HBWKey::loop(HBWDevice* device, uint8_t channel) {
  
  if (!config->n_input_locked)  return;  // skip locked channels
  
  uint32_t now = millis();
  if (now == 0) now = 1;  // do not allow time=0 for the below code // AKA  "der Teufel ist ein Eichhoernchen"
  
  bool buttonState = (digitalRead(pin) ^ !config->n_inverted);
  buttonState = activeHigh ^ buttonState;
  
  switch (config->input_type) {
    case DOORSENSOR:
  // sends a short KeyEvent on HIGH and long KeyEvent on LOW input level changes
      if (buttonState != oldButtonState) {
        if (!keyPressedMillis) {
          keyPressedMillis = now;
        }
        else if (now - keyPressedMillis >= DOORSENSOR_DEBOUNCE_TIME) {
          if ( (keyPressNum & 0x3F) == 0 ) keyPressNum = 1;  // do not send keyNum=0
          if (device->sendKeyEvent(channel, keyPressNum, !buttonState) != HBWDevice::BUS_BUSY) {
            keyPressNum++;
            oldButtonState = buttonState;
          }
        }
      }
      else {
        keyPressedMillis = 0;
      }
      break;
  
    case SWITCH:
  // sends a short KeyEvent, each time the switch changes the polarity
      if (buttonState) {
        if (!keyPressedMillis) {
         // Taste war vorher nicht gedrueckt
         keyPressedMillis = now;
        }
        else if (now - keyPressedMillis >= SWITCH_DEBOUNCE_TIME && !lastSentLong) {
          keyPressNum++;
          device->sendKeyEvent(channel, keyPressNum, false);
          lastSentLong = now;
        }
      }
      else {
        if (lastSentLong) {
          keyPressNum++;
          device->sendKeyEvent(channel, keyPressNum, false);
          lastSentLong = 0;
        }
        keyPressedMillis = 0;
      }
      break;
    
    case PUSHBUTTON:
  // sends short KeyEvent on short press and (repeated) long KeyEvent on long press
      if (buttonState) {
        // d.h. Taste nicht gedrueckt
        // "Taste war auch vorher nicht gedrueckt" kann ignoriert werden
          // Taste war vorher gedrueckt?
        if (keyPressedMillis) {
          // entprellen, nur senden, wenn laenger als 50ms gedrueckt
          // aber noch kein "long" gesendet
          if (now - keyPressedMillis >= KEY_DEBOUNCE_TIME && !lastSentLong) {
            keyPressNum++;
            device->sendKeyEvent(channel, keyPressNum, false);
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
            if (now - lastSentLong >= 300) {  // alle 300ms wiederholen
              // keyPressNum nicht erhoehen
              device->sendKeyEvent(channel, keyPressNum, true);  // long press
              lastSentLong = now;
            }
          }
          else if (now - keyPressedMillis >= long(config->long_press_time) * 100) {
            // erstes LONG
            keyPressNum++;
            device->sendKeyEvent(channel, keyPressNum, true);  // long press
            lastSentLong = now;
          };
        }
        else {
          // Taste war vorher nicht gedrueckt
          keyPressedMillis = now;
          lastSentLong = 0;
        }
      }
      break;
  
    case MOTIONSENSOR:
  // sends a short KeyEvent for raising or falling edge - not both
      if (buttonState) {
        if (!keyPressedMillis) {
         // Taste war vorher nicht gedrueckt
         keyPressedMillis = now;
        }
        else if (now - keyPressedMillis >= SWITCH_DEBOUNCE_TIME && !lastSentLong) {
          if ( (keyPressNum & 0x3F) == 0 ) keyPressNum = 1;  // do not send keyNum=0
          if (device->sendKeyEvent(channel, keyPressNum, false) != HBWDevice::BUS_BUSY) {
            keyPressNum++;
            lastSentLong = now;
          }
        }
      }
      else {
        if (lastSentLong) {
          lastSentLong = 0;
        }
        keyPressedMillis = 0;
      }
      break; 
  }
};
