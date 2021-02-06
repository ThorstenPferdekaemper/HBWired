/*
 * HBWDimBacklight.h
 *
 * Created on: 16.07.2020
 * loetmeister.de
 * No commercial use!
 *
 * Dimmable backlight for displays or other ilumination
 * Set level 0..100%, auto_brightness & auto_off (1..17 minutes)
 *
 */
 
#ifndef HBWDIMMERSIMPLE_H_
#define HBWDIMMERSIMPLE_H_


#include "HBWired.h"


#define DEBUG_OUTPUT   // debug output on serial/USB


// config of Display backlight (dimmer) channel, address step 2
struct hbw_config_dim_backlight {
  uint8_t startup:1;  // initial state, 1=ON
  uint8_t auto_brightness:1;  // default 1=auto_brightness enabled
  uint8_t auto_off:4;  // 1..17 minutes standby delay. 0 = always on ( 0 = "special value", NOT_USED)
  uint8_t :2;     //fillup
  uint8_t dummy;
};


// dimmer channel for display backlight (or overall brightness, e.g. for OLED?)
// Class HBWDimBacklight
class HBWDimBacklight : public HBWChannel {
  public:
//    HBWDimBacklight(hbw_config_dim_backlight* _config, uint8_t _backlight_pin, HBWChannel* _brightnessChan = NULL);
    HBWDimBacklight(hbw_config_dim_backlight* _config, uint8_t _backlight_pin, uint8_t _photoresistor_pin = NOT_A_PIN);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);

    static boolean displayWakeUp;   // allow other channels to "wake up" the backlight
  
  private:
    hbw_config_dim_backlight* config;
    uint8_t backlightPin;  // (PWM!) pin for backlight
    uint8_t photoresistorPin;  // light resistor (mabye not used == NOT_A_PIN) - pin must be ADC!
    uint8_t brightness;   // read ADC and save average here
//    HBWChannel* brightnessChan;
    uint8_t currentValue;   // current dimmer level (0...200 -> 0...100%)
    uint32_t backlightLastUpdate;    // last time of update
    uint32_t powerOnTime;
    uint8_t lastKeyNum;
    boolean initDone;
    
    static const uint32_t BACKLIGHT_UPDATE_INTERVAL = 1660;

    // TODO: add notify/logging? Not really needed, should be disabled in auto_brightness mode anyway
};


#endif /* HBWDIMMERSIMPLE_H_ */
