/*
 * HBWKey.cpp
 *
 * Updated (www.loetmeister.de): 29.09.2018
 * - Added: input_locked, n_inverted, input_type (SWITCH, MOTIONSENSOR, DOORSENSOR)
 * - Changes require new XML layout for config!
 * 
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
  hbwdebug(F("\n"));
#endif
};


void HBWKey::loop(HBWDevice* device, uint8_t channel) {
  
  uint32_t now = millis();
  
  if (config->n_input_locked) {   // skip locked channels
    
    bool buttonState = (digitalRead(pin) ^ !config->n_inverted);
    buttonState = activeHigh ^ buttonState;
    
    switch (config->input_type) {
      case IN_SWITCH:
    // sends a short KeyEvent, each time the switch changes the polarity
        if (buttonState) {
          if (!keyPressedMillis) {
           // Taste war vorher nicht gedrueckt
           keyPressedMillis = now ? now : 1;
          }
          else if (now - keyPressedMillis >= SWITCH_DEBOUNCE_TIME && !lastSentLong) {
            keyPressNum++;
            lastSentLong = now ? now : 1;
            device->sendKeyEvent(channel, keyPressNum, false);
          }
        }
        else {
          if (lastSentLong) {
            keyPressNum++;
            lastSentLong = 0;
            device->sendKeyEvent(channel, keyPressNum, false);
          }
          keyPressedMillis = 0;
        }
        break;
      
      case IN_PUSHBUTTON:
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
                lastSentLong = now ? now : 1; // der Teufel ist ein Eichhoernchen
                device->sendKeyEvent(channel, keyPressNum, true);  // long press
              }
            }
            else if (now - keyPressedMillis >= long(config->long_press_time) * 100) {
              // erstes LONG
              keyPressNum++;
              lastSentLong = now ? now : 1;
              device->sendKeyEvent(channel, keyPressNum, true);  // long press
            };
          }
          else {
            // Taste war vorher nicht gedrueckt
            keyPressedMillis = now ? now : 1; // der Teufel ist ein Eichhoernchen
            lastSentLong = 0;
          }
        }
        break;
    
      case IN_MOTIONSENSOR:
    // sends a short KeyEvent for raising or falling edge - not both
        if (buttonState) {
          if (!keyPressedMillis) {
           // Taste war vorher nicht gedrueckt
           keyPressedMillis = now ? now : 1;
          }
          else if (now - keyPressedMillis >= SWITCH_DEBOUNCE_TIME && !lastSentLong) {
            keyPressNum++;
            lastSentLong = now ? now : 1;
            device->sendKeyEvent(channel, keyPressNum, false);
          }
        }
        else {
          if (lastSentLong) {
            lastSentLong = 0;
          }
          keyPressedMillis = 0;
        }
        break;
    
#ifdef IN_DOORSENSOR
      case IN_DOORSENSOR:
    // sends a short KeyEvent on HIGH and long KeyEvent on LOW input level changes
        if (buttonState != oldButtonState) {
          if (!keyPressedMillis) {
            keyPressedMillis = now ? now : 1;
          }
          else if (now - keyPressedMillis >= DOORSENSOR_DEBOUNCE_TIME) {
            keyPressNum++;
            oldButtonState = buttonState;
			// TODO: if bus is not idle, retry next time (sendKeyEvent must retun correct state before this is possible to implement!!)
            device->sendKeyEvent(channel, keyPressNum, !buttonState);
          }
        }
        else {
          keyPressedMillis = 0;
        }
        break;
#endif
   }
  }
};
