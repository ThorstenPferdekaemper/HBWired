/*
 * HBWDimBacklight.cpp
 *
 * Created on: 16.07.2020
 * loetmeister.de
 * 
 */
 
#include "HBWDimBacklight.h"


/* global/static */
boolean HBWDimBacklight::displayWakeUp = true;  // wakeup on start - if config->startup allows


HBWDimBacklight::HBWDimBacklight(hbw_config_dim_backlight* _config, uint8_t _backlight_pin, uint8_t _photoresistor_pin)
{
  config = _config;
  backlightPin = _backlight_pin;
  photoresistorPin = _photoresistor_pin;
  backlightLastUpdate = 0;
  lastKeyNum = 0;
  initDone = false;
  currentValue = 0;
};

// TODO? Replace _photoresistor_pin with seperate brightness channel (i.e. Analog in? ... but we need 0-200 pct values as auto_brightness input!)
//HBWDimBacklight::HBWDimBacklight(hbw_config_dim_backlight* _config, uint8_t _backlight_pin, HBWChannel* _brightnessChan)
//{
//  config = _config;
//  backlightPin = _backlight_pin;
//  //photoresistorPin = _photoresistor_pin;
//  brightnessChan = _brightnessChan;
//  backlightLastUpdate = 0;
//  lastKeyNum = 0;
//  initDone = false;
//  currentValue = 0;
//};

/* class functions */

/* standard public function - set a channel, directly or via peering event. Data array contains new value or all peering details */
  // 0 - backlight off (0%)
  // 1...199 - 0.5% - 99.5% -> auto_brightness to 'off' when setting explicit value
  // 200 - backlight on (100%)
  // 202 - auto_brightness off
  // 204 - auto_brightness on
  // 255 - toggle backligh
void HBWDimBacklight::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
 #ifdef DEBUG_OUTPUT
 hbwdebug(F("setDim: "));
 #endif

  if (length == 2)
  {
    if (lastKeyNum == data[1])  // ignore repeated key press
      return;
    else
      lastKeyNum = data[1];
  }

  HBWDimBacklight::displayWakeUp = true;
  
  if (data[0] > 200)
  {
    switch (data[0]) {
      case 202:  // auto_brightness off
        config->auto_brightness = false;
   #ifdef DEBUG_OUTPUT
   hbwdebug(F("auto_brightness off"));
   #endif
        break;
      case 204:  // auto_brightness on
        config->auto_brightness = true;
   #ifdef DEBUG_OUTPUT
   hbwdebug(F("auto_brightness on"));
   #endif
        break;
      case 255:   // toggle on/off, only when not in auto_brightness
        if (config->auto_brightness == false)
          currentValue = currentValue > 0 ? 0 : 200;
   #ifdef DEBUG_OUTPUT
   hbwdebug(F("toggle"));
   #endif
        break;
    }
  }
  else {
    currentValue = data[0];
    config->auto_brightness = false;  // auto_brightness off when setting value manually
  }
  
#ifdef DEBUG_OUTPUT
hbwdebug(F(", currentValue: ")); hbwdebug(currentValue);
hbwdebug(F("\n"));
#endif 
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWDimBacklight::get(uint8_t* data)
{
  *data = currentValue;

  return 1;
};


/* standard public function - called by device main loop for every channel in sequential order */
void HBWDimBacklight::loop(HBWDevice* device, uint8_t channel)
{
  if (!initDone) {
    if (config->startup)  currentValue = 200; // set startup value (200 = on, 0 = off)
    initDone = true;
    return;
  }

  uint32_t now = millis();

  if (now - backlightLastUpdate > BACKLIGHT_UPDATE_INTERVAL || displayWakeUp)
  {
    backlightLastUpdate = now;
    
    if (displayWakeUp) {
      displayWakeUp = false;
      powerOnTime = now;   // reset timer for "auto_off"
    }

    if (config->auto_brightness && photoresistorPin != NOT_A_PIN)
    {
      int16_t brightnessDiff = (analogRead(photoresistorPin) >> 2) - brightness;
      
      // smooth ADC reading, but allow larger steps if backlight is bright, or the measured brightness changed a lot
      if (brightnessDiff > 3) {
        brightness += 1 + (currentValue/32) + (brightnessDiff/50);  // try using divider with power of 2, to use bitshift
      }
      else if (brightnessDiff < -3) {
        brightness -= 1 + (currentValue/32) + ((brightnessDiff * -1)/50);
      }
      
      currentValue = map(brightness, 0, 255, 0, 200);
    }

    // overwrite current value, to turn of the backlight, if auto-off applies
    if (now - powerOnTime >= (uint32_t)config->auto_off *60000 && config->auto_off)
      currentValue = 0;

//hbwdebug(F("Dim LDR:")); hbwdebug(analogRead(photoresistorPin) >> 2);
//hbwdebug(F(" brightness:")); hbwdebug(brightness);
//hbwdebug(F(" currentValue:")); hbwdebug(currentValue);
//hbwdebug(F("\n"));
    
    analogWrite(backlightPin, map(currentValue, 0, 200, 0, 255));
  }
};


// TODO: adapt for brightnessChan?
//void HBWDimBacklight::loop(HBWDevice* device, uint8_t channel)
//{
//  if (!initDone) {
//    if (config->startup)  currentValue = 200; // set startup value (200 = on, 0 = off)
//    initDone = true;
//    return;
//  }
//
//  uint32_t now = millis();
//
//  if (now - backlightLastUpdate > BACKLIGHT_UPDATE_INTERVAL || displayWakeUp)
//  {
//    backlightLastUpdate = now;
//    
//    if (displayWakeUp) {
//      displayWakeUp = false;
//      powerOnTime = now;   // reset timer for "auto_off"
//    }
//
//    if (config->auto_brightness && brightnessChan != NULL)
//    {
//      uint8_t reading;
//      brightnessChan->get(&reading);
//      int16_t brightnessDiff = reading - brightness;
//      
//      // smooth ADC reading, but allow larger steps if backlight is bright, or the measured brightness changed a lot
//      if (brightnessDiff > 3) {
//        brightness += 1 + (currentValue/32) + (brightnessDiff/40);  // try using divider with power of 2, to use bitshift
//      }
//      else if (brightnessDiff < -3) {
//        brightness -= 1 + (currentValue/32) + ((brightnessDiff * -1)/40);
//      }
//      
//      currentValue = brightness;
//    }
//
//    // overwrite current value, to turn of the backlight, if auto-off applies
//    if (now - powerOnTime >= (uint32_t)config->auto_off *60000 && config->auto_off)
//      currentValue = 0;
//
////hbwdebug(F("Dim LDR:")); hbwdebug(analogRead(photoresistorPin) >> 2);
////hbwdebug(F(" brightness:")); hbwdebug(brightness);
////hbwdebug(F(" currentValue:")); hbwdebug(currentValue);
////hbwdebug(F("\n"));
//    
//    analogWrite(backlightPin, map(currentValue, 0, 200, 0, 255));
//  }
//};
