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

// To play buzzer tones
#include "pitches.h"

// #define DEBUG_OUTPUT


// config, address step 5
struct hbw_config_key_doorbell {
  uint8_t n_input_locked:1;   // +0.0    0=LOCKED, 1=UNLOCKED
  uint8_t n_inverted:1;       // +0.1    0=inverted, 1=not inverted
  uint8_t pullup:1;           // +0.2 // TODO: needed....?
  uint8_t repeat_long_press:1;  // allow long press KeyEvent for depressed key (press and hold) - send again after suppress_time passed
  // uint8_t       :3;           // 0x07:5-7
  uint8_t long_press_time;    // +1.0
  // key blocking: first key press will always be send. Next one after 'suppress_num' count was reached or no key was pressed during 'suppress_time'
  // long_press will be repeated every 'suppress_time' (unless disabled by repeat_long_press)
  uint8_t suppress_num:4;    // +2.0   amount of presses that will be suppressed (0-14)
  uint8_t :4;
  uint8_t suppress_time:4;   // +3.0   blocking time for repeated key presses (0-14 sec)
  uint8_t buzzer:3;          // +3.4 buzzer config (TODO: melody1, melody2, melody3, melody4), disabled=0, 7=default melody
  uint8_t fillup:1;
  uint8_t dummy;
  // union dial_string {
  //   char [18] phone_number;
  //   char dial_button;
  // };
};

#define NUM_NOTES 3
// note=0 creates silence. length=1000 skips the note (dealy 1.3ms), length=0 stops the melody (starting with 0 will disable the whole melody!)
// fields: melody[melody 'pair'][fail|ok][note|note length][note]
PROGMEM const int melody[][2][2][NUM_NOTES] = {
  { { {NOTE_A3, NOTE_C3, 0}, {8, 4, 0} }, { {0, NOTE_B5, NOTE_E6}, {1000, 16, 8} } },  // default melody
  { { {NOTE_A3, NOTE_C3, 0}, {8, 4, 0} }, { {NOTE_G4, NOTE_F4, NOTE_D5}, {16, 8, 0} } },  // melody1
  { { {NOTE_A3, NOTE_C3, 0}, {8, 4, 0} }, { {NOTE_B5, NOTE_E6, 0}, {16, 8, 0} } }  // melody2 dummy
};


// Class HBWKeyDoorbell
class HBWKeyDoorbell : public HBWChannel {
  public:
    HBWKeyDoorbell(uint8_t _pin, hbw_config_key_doorbell* _config, uint8_t _pinBuzzer = NOT_A_PIN, bool _activeHigh = false);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void afterReadConfig();
    

  private:
    hbw_config_key_doorbell* config;
    uint8_t pin;   // input pin
    uint32_t keyPressedMillis;  // Zeit, zu der die Taste gedrueckt wurde (fuer's Entprellen)
    uint32_t lastSentLong;      // Zeit, zu der das letzte Mal longPress gesendet wurde
    uint8_t keyPressNum;
    uint8_t repeatCounter;
    uint32_t lastKeyPressedMillis;  // last press of any key
    bool activeHigh;    // activeHigh=true -> input active high, else active low

    static const uint32_t KEY_DEBOUNCE_TIME = 85;  // ms

    enum buzzerAction {
      BLOCKED = 0,  // must match with melody array index for the 'fail/ok' field (0/1)
      SUCCESS,
      PLAY
    };
    enum {
      NOTE = 0,  // must match with melody array index for the 'note/note length' field (0/1)
      NOTE_LEN
    };
    uint8_t pinBuzzer;   // pin for the buzzer - if used
    void buzzer(uint8_t _action, bool _forceChange = false);
    static const uint8_t MAX_MELODY_CONFIG = 7;  // need to match with device XML
};

#endif
