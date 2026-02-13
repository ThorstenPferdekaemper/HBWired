/*
 * HBWPhoneDial.cpp
 *
 * Created (www.loetmeister.de): 25.01.2026
 */

#include "HBWPhoneDial.h"


// Class HBWPhoneDial
HBWPhoneDial::HBWPhoneDial(hbw_config_phone_dial* _config, SHIFT_REGISTER_CLASS* _shiftRegister, uint8_t _chan_offset, uint8_t _lineStatePin)
{
  config = _config;
  shiftRegister = _shiftRegister;
  lineStatePin = _lineStatePin;
  channelOffset = _chan_offset;
  lineState = line_status::unknown;
  phoneState = phone_status::onHook;  // assume idle... lineState will be checked anyway

  pinMode(lineStatePin, INPUT_PULLUP);
  disabled = false;
};


void HBWPhoneDial::afterReadConfig()
{
  // if (config->suppress_time == 0xF) config->suppress_time = 3;  // 3 seconds
  // if (!config->enabled) disabled = true;

#ifdef DEBUG_OUTPUT_PD
  hbwdebug(F("cfg DB_Dial LinePin:"));
  hbwdebug(lineStatePin);
  hbwdebug(F(" chOffset:"));
  hbwdebug(channelOffset);
  // hbwdebug(F(" pullup:"));
  // hbwdebug(config->pullup);
  hbwdebug(F("\n"));
#endif
};


uint8_t HBWPhoneDial::get(uint8_t* data)
{
  *data++ = lineState;  // Show hook state as channel on/off
  *data = (phoneState <<4); // use state flags for phoneState (3 bits starting at 4th bit, i.e. values 0-7; see XML)

	return 2;
};


void HBWPhoneDial::set(HBWDevice* device, uint8_t length, uint8_t const * const data)
{
  if (*data == 0) {
    disabled = true;  // temporarly disable. Does not terminate an ongoing call, but blocks any new ones
  }
  else if (*data <= 8) {
    DialNumber(*data);  // trigger configured action (key 1-8 -> ch:0-7) - unless disabled
    // TODO: consider channel offeset?
  }
  else if (*data == 200) {
    disabled = false;  // enable again (no effect when also disabled by config)
  }
  else if (*data == 250) {  // RESET
    phoneState = phone_status::reset;
    setNewPhoneTimer();  // clear timer
  }
  else if (*data == 254 && phoneState != phone_status::onHook) {  // HANGUP
    phoneState = phone_status::hangupWait;  // "presses" hook key (works in temp disabled state)
    setNewPhoneTimer();  // clear timer
  }
  else if (*data == 255) {  // HOOK TOGGLE
    phoneState = phone_status::hangupWait;  // toggles hook key (works in temp disabled state)
    setNewPhoneTimer();  // clear timer
  }
};


void HBWPhoneDial::loop(HBWDevice* device, uint8_t channel)
{
  if (!config->enabled) {
    disabled = true;  // this disables also DialNumber();
    lineState = line_status::idle;
    phoneState = phone_status::onHook;  // we don't care about the actual phone state
    return;
  }

  phoneLoop(); // handle phone actions

  if (!disabled)  lineState = checkLineState();  // check the status of the phone line

#ifdef DEBUG_OUTPUT_PD
  static unsigned char lineStateOld = line_status::unknown;
  if (lineState != lineStateOld) { hbwdebug(F("lineState: ")); hbwdebug(lineState); hbwdebug(F("\n")); lineStateOld = lineState; }
#endif

};


/* initate dialing/calling from other channels of the device */
bool HBWPhoneDial::DialNumber(uint8_t _index_num)
{
  byte chan_calc = _index_num - channelOffset;
  hbwdebug(F("triggerDial, ch: ")); hbwdebug(chan_calc);
  if (chan_calc > (uint8_t)sizeof(config->db_mapping)) { hbwdebug(F(" mismatch!\n")); return false; }  // out of range
  if (config->db_mapping[chan_calc].max_call_duration == 0x1F) { hbwdebug(F(" disabled!\n")); return false; }  // disabled

  if (phoneState != phone_status::onHook || lineState != line_status::idle || disabled) {
    hbwdebug(F(" - busy. DialDelay: ")); hbwdebug(phoneActionDelay);
    hbwdebug(F(" remaining: ")); hbwdebug(phoneActionDelay - (millis() - phonePreviousMillis));
    hbwdebug(F("\n"));
    return false;  // already dialed or active call
  }
  phoneState = phone_status::dialing;
  dbChannel = chan_calc; // store the channel number by which we got triggered
  setNewPhoneTimer();  // clear timer
  hbwdebug(F(" - ok\n"));
  return true;
};


void HBWPhoneDial::phoneLoop()
{
  if (millis() - phonePreviousMillis >= phoneActionDelay)
  {
    setNewPhoneTimer();  // clear timer

    if (phoneState == phone_status::dialing) {
      // const char num[] = {"123456"};
      // phoneState = dial(num);

      // TODO: create a way to retreive strings from EEPROM - right now, can only use quick dial
      char quickDial[] = {"1"};  // create with dummy value... 2 byte string
      quickDial[0] = (config->db_mapping[dbChannel].db_map +1) + '0';  // lookup quick dial number and convert to string
      phoneState = dial(quickDial);
    }
    else if (phoneState == phone_status::hangupWait || lineState == line_status::terminated) {
      // max call time reached or call terminated
      const char num_hook[] = {"7"};  // temp, fixed hook key index number
      phoneState = dial(num_hook);
    }
    else if (phoneState == phone_status::reset) {
      const char num_reset[] = {"8"};  // temp, fixed reset key index number
      phoneState = dial(num_reset);
    }
  }
};

uint8_t HBWPhoneDial::dial(const char* _numStr)//, bool quickDial)
{
  static uint16_t _dialKeyPressAndPauseDelay = 0;
  static unsigned long _dialPreviousMillis = 0;
  static bool _buttonPressed = false;
  static unsigned char _i = 0;
  static unsigned char _n;

  if (millis() - _dialPreviousMillis >= _dialKeyPressAndPauseDelay) {
    _dialPreviousMillis = millis();

    // add if (specialButton) ? // like hook or quick dial?

    if (_buttonPressed) {
      // release button again and wait configured time
      shiftRegister->setAllLow();
      _buttonPressed = false;
      _dialKeyPressAndPauseDelay = DIAL_PRESS_PAUSE_TIME;
      // if (_numStr[_i] == '<') _dialKeyPressAndPauseDelay = OFF_HOOK_PAUSE_TIME; // fixed pause time... should wait for line signal...?!
      if (_numStr[_i] == '7')  _dialKeyPressAndPauseDelay = OFF_HOOK_PAUSE_TIME;
      if (_i <= strlen(_numStr)) _i++;
      if (_i >= strlen(_numStr)) {  // end of number/dial string
        _i = 0;
        _n = 0;
        _dialKeyPressAndPauseDelay = 0;

        if (phoneState == phone_status::dialing) {
          setNewPhoneTimer((unsigned long)(config->db_mapping[dbChannel].max_call_duration +1) * 10000);
          hbwdebug(F("dial done\n"));
          return phone_status::hangupWait;
        }
        else if (phoneState == phone_status::hangupWait || lineState == line_status::terminated) {
          // lineState = line_status::idle;
          lineState = line_status::unknown;  // will be checked and set correctly by checkLineState()
          hbwdebug(F("onHook\n"));
          return phone_status::onHook;
        }
        else if (phoneState == phone_status::reset) {
          hbwdebug(F("reset done!\n"));
          return phone_status::onHook;
        }
      }
    }
    else {
      // press and hold for configured time
      _buttonPressed = true;
      _dialKeyPressAndPauseDelay = DIAL_PRESS_TIME;
      _n = _numStr[_i] - '0';
      if (_n == 8) _dialKeyPressAndPauseDelay = RESET_DURATION;  // temp, fixed reset key index number
      if (_n < sizeof(PhoneHotkeys)) {
      // if (_n < sizeof(PhoneButton)) {
        // shiftRegister->setAll_P(&PhoneButton[n]);
        shiftRegister->setAll_P(&PhoneHotkeys[_n]);
        _dialPreviousMillis = millis();  // update timestamp after shift register had been set
        hbwdebug(F("Dialed: "));//hbwdebug(_numStr[_i]);hbwdebug(F(" arNum: "));hbwdebug(_n);hbwdebug(F("\n"));
      }
      else {
        hbwdebug(F("ERR Dial: "));
      }  hbwdebug(_numStr[_i]);hbwdebug(F(" arNum: "));hbwdebug(_n);hbwdebug(F("\n"));
    }
  }
  return phoneState;  // no state change
};

/* check the input pin of the phone line indicator - update 'lineState' based on this indicator and the current call status (phoneState) */
uint8_t HBWPhoneDial::checkLineState()
{
  if (lineStatePin == NOT_A_PIN) {
    return line_status::idle;  // return idle when not used, to still allow calls
  }
  // pin debounce:
  static bool line;
  line = lineIsActive();
  static bool previousLine = !line; // init with opposite value
  static unsigned long lineStateCheckMillis = 0;

  if (line == previousLine) {
    if (!(millis() - lineStateCheckMillis >= LINE_CHANGE_DEBOUNCE))  return lineState;
  }
  else {
    previousLine = line;
    lineStateCheckMillis = millis();  // restart timer
    return lineState;
  }

  // check the status of the phone line
  switch (lineState)
  {
    case line_status::idle: {
      if ((phoneState == phone_status::dialing || phoneState == phone_status::hangupWait) && lineIsActive())  return line_status::offHookDialing;
      else if (phoneState == phone_status::onHook && lineIsActive())  return line_status::lineError; // active but we did not even start; TODO set error state and recover (reset phone?)
      }
      break;
    case line_status::offHookDialing: {
      // wait for lineStatePin to become HIGH (active but reversed poliarity)
      if (phoneState == phone_status::hangupWait && lineIsReversedPolarity())  return line_status::callEstablished;
      }
      break;
    case line_status::callEstablished: {
      // wait for lineStatePin to become active again (LOW)
      if (phoneState == phone_status::hangupWait && lineIsActive()) {
        setNewPhoneTimer(200);  // need short pause before pressing hook key after termination was detected!
        return line_status::terminated;
      }
      if (phoneState == phone_status::onHook && !lineIsActive())  return line_status::idle;  // reset line state when on hook again (max call duration reached)
      }
      break;
    case line_status::terminated: {
        if (phoneState == phone_status::onHook && !lineIsActive())  return line_status::idle;
      }
      break;
    case line_status::lineError: {
      phoneState = phone_status::reset;      // TODO: add some delay? (avoid reset for short hickup?)
      setNewPhoneTimer();  // clear timer
      // setNewPhoneTimer(ON_HOOK_PAUSE_TIME);
      return line_status::unknown;  // if line is still active after reset, we remain in unknown state until line becomms clear
      }
      break;
    case line_status::unknown: {
          if (!lineIsActive()) return line_status::idle;
      }
      break;
    // default:
    //   return line_status::unknown;
  }
  return lineState;  // no change
};
