/*
 * HBWKeyDoorbell.cpp
 *
 * Created (www.loetmeister.de): 25.01.2020
 * 
 * build for doorbell pushbuttons
 * allows to block for a specific time, if someone keeps ringing
 */

#ifndef HBWKeyDoorbell_h
#define HBWKeyDoorbell_h

#include <inttypes.h>
#include "HBWired.h"


#define DEBUG_OUTPUT



// config, address step 4
struct hbw_config_key_doorbell {
  uint8_t n_input_locked:1;   // +0.0    0=LOCKED, 1=UNLOCKED
  uint8_t n_inverted:1;       // +0.1    0=inverted, 1=not inverted
  uint8_t pullup:1;           // +0.2 // TODO: needed....?
  uint8_t repeat_long_press:1;  // allow long press KeyEvent for depressed key (press and hold) - send again after suppress_time passed
  uint8_t       :3;           // 0x07:5-7
  uint8_t long_press_time;    // +1.0
  // key blocking: first key press will always be send. Next one after 'suppress_num' count was reached or no key was pressed for 'suppress_time'
  // long_press will be repeated every 'suppress_time' (unless disabled by repeat_long_press)
  uint8_t suppress_num;    // +2.0   amount of presses that will be suppressed // TODO: 4 bit sufficent?
  uint8_t suppress_time;   // +3.0   blocking time for repeated key presses  // TODO: 4 bit sufficent? (500ms stepping?)
};


// Class HBWKeyDoorbell
class HBWKeyDoorbell : public HBWChannel {
  public:
    HBWKeyDoorbell(uint8_t _pin, hbw_config_key_doorbell* _config, boolean _activeHigh = false);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void afterReadConfig();
    

  private:
    uint8_t pin;   // input pin
    uint32_t keyPressedMillis;  // Zeit, zu der die Taste gedrueckt wurde (fuer's Entprellen)
    uint32_t lastSentLong;      // Zeit, zu der das letzte Mal longPress gesendet wurde
    uint8_t keyPressNum;
    uint8_t repeatCounter;
    uint32_t lastKeyPressedMillis;  // last press of any key
    hbw_config_key_doorbell* config;
    boolean buttonState;
    boolean oldButtonState;
    boolean activeHigh;    // activeHigh=true -> input active high, else active low
    
    static const uint32_t KEY_DEBOUNCE_TIME = 85;  // ms
};

#endif
