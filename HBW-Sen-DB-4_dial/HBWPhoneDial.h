/*
 * HBWPhoneDial.h
 *
 * Created (www.loetmeister.de): 25.01.2026
 * 
 * Interface to telephone (num pad & line monitoring)
 * Dials specfic numbers. One call at a time.
 */

#ifndef HBWPhoneDial_h
#define HBWPhoneDial_h

#include <inttypes.h>
#include "HBWired.h"
#include <HBWKeyDoorbell.h>
#include "src/ShiftRegister74HC595.h" // shift register library
//Notice the 0.1"f capacitor on the latchPin ?

// #define DEBUG_OUTPUT_PD

#define SHIFT_REGISTER_CLASS ShiftRegister74HC595<1>  // 1 shift register


struct s_db_dial_chan {
  uint8_t db_map:  3;      // 0-7 = (quick) dial numbers 1...8
  uint8_t max_call_duration:5; //  max duration. default disabled == 0x1F, 10 seconds stepping
};

// config, address step 12
// TODO: ?  save phone_numbers in own EE space? same as LCD module?
// or save numbers in phone? (quick dial)
struct hbw_config_phone_dial {
  uint8_t enabled:1;   // "master" channel enable option
  uint8_t volume:2;   // default volume? (-2 ... default +2?)  // TODO: implement somehow
  uint8_t fillup:5;   // 
  uint16_t dummy;
  // db-chan mapping to phone numbers / quick dial numbers?
  s_db_dial_chan db_mapping[4];
  uint32_t dummy2;  // block 4 bytes, to allow 8 DB channels...
  // TODO: add real phone numbers...
  // char* num[4] pointer to phone number string?
  // union dial_string {
  //   char [18] phone_number;
  //   char dial_button;
  // };
  uint8_t unused;
};


// dial numbers/buttons via serial shift register (attached to button matrix - rows/columns)
// PhoneButton[0...9] -> 0...9, PhoneButton[10] -> '*', PhoneButton[12] -> off/on hook
PROGMEM const unsigned char PhoneButton[] = {
//byte pos: R1|R2|R3|R4|C4|C3|C2|C1
  0b00010010, // 0
  0b10000001, // 1
  0b10000010, // 2
  0b10000100, // 3
  0b01000001, // 4
  0b01000010, // 5
  0b01000100, // 6
  0b00100001, // 7
  0b00100010, // 8
  0b00100100, // 9
  0b00010001, // *
  0b00010100, // #
  0b00011000, // on/off hook
  0b01001000, // vol-
  0b00101000  // vol+
};

//TODO: use above PhoneButton[] instead to allow full control of the phone
// quick dial buttons and others via serial shift register
PROGMEM const unsigned char PhoneHotkeys[] = {
  0b00000000, // 0 - unused
  0b10000000, // 1, quick dial
  0b01000000, // 2, quick dial
  0b00100000, // 3, quick dial
  0b00010000, // 4, quick dial
  0b00000000, // 5 - unused
  0b00000000, // 6 - unused
  0b00000100, // on/off hook (7)
  0b00001000  // reset phone (8)
};


// Class HBWPhoneDial
class HBWPhoneDial : public HBWPhoneDial_Base {
  public:
    HBWPhoneDial(hbw_config_phone_dial* _config, SHIFT_REGISTER_CLASS* _shiftRegister, uint8_t _chan_offset = 0, uint8_t _lineStatePin = NOT_A_PIN);
    virtual void loop(HBWDevice*, uint8_t channel);
	// TODO set() // hangup / reset line, volume up/down commands? Or allow set number and store in EEPROM / phone??
    virtual void set(HBWDevice* device, uint8_t length, uint8_t const * const data);
    virtual uint8_t get(uint8_t* data);
    virtual void afterReadConfig();
    
    bool DialNumber(uint8_t _index_num);
    
  private:
    hbw_config_phone_dial* config;
    SHIFT_REGISTER_CLASS* shiftRegister;  // allow function calls to the shift register
    // uint8_t resetPin; // output pin for phone reset - if present - part of shift register
    uint8_t lineStatePin; // input pin for phone line state - if present
    uint8_t lineState;  // phone line state (reverse polarity upon call establishment and termination)
    uint8_t channelOffset;
    uint8_t dbChannel;
    void phoneLoop();
    uint8_t checkLineState(void);
    uint8_t dial(const char* _numStr);
    uint8_t phoneState;
    unsigned long phonePreviousMillis;
    unsigned long phoneActionDelay;
    bool disabled;

    static const uint16_t DIAL_PRESS_TIME = 100;  // ms
    static const uint16_t DIAL_PRESS_PAUSE_TIME = 400;  // ms
    // static const uint16_t QUICK_DIAL_PRESS_TIME = 500;  // 500 ms for dialing short keys
    static const uint16_t OFF_HOOK_PAUSE_TIME = 1200;  // ms
    // static const uint16_t ON_HOOK_PAUSE_TIME = 2000;  // ms - wait after call was ended (give SIP device time to close the call)
    // static const uint16_t HANGUP_AFTER = 26;  // seconds
    static const uint16_t RESET_DURATION = 4000;  // ms (power cycle the connected phone)
    static const uint16_t LINE_CHANGE_DEBOUNCE = 66;

    enum phone_status {
      onHook = 0,
      dialing,
      hangupWait,
      reset
    };

    enum line_status {
      idle = 0,  // unknown or idle
      offHookDialing,
      callEstablished,
      terminated,
      lineError,
      unknown = 255
    };

    enum phone_buttons {
      asterisk = 10,
      hash,
      hook,
      volDown,
      volUp,
      quickDial = 200
    };

    // uint16_t getButtonWaitTime(bool _depressed, uint8_t _specialButton)
    // {
    //   if (_specialButton == 0) {
    //     return _depressed ? DIAL_PRESS_TIME : DIAL_PRESS_PAUSE_TIME;
    //   }
    //   else if (_specialButton == phone_buttons::hook) {
    //     return _depressed ? DIAL_PRESS_TIME : OFF_HOOK_PAUSE_TIME;
    //   }
    //   else if (_specialButton == phone_buttons::quickDial) {
    //     return _depressed ? QUICK_DIAL_PRESS_TIME : DIAL_PRESS_PAUSE_TIME;
    //   }
    // };

    void setNewPhoneTimer(unsigned long _newTime = 0) {
      phoneActionDelay = _newTime;
      phonePreviousMillis = millis();
    }

    bool lineIsActive()  { return digitalRead(lineStatePin) == false; }  // pulled low when active
    bool lineIsReversedPolarity()  { return digitalRead(lineStatePin) == true; }  // high when polarity is reversed (or when line is idle)
    
};

#endif
