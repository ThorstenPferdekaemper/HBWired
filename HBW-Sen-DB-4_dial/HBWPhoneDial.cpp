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
};

void HBWPhoneDial::afterReadConfig()
{
  // if (config->suppress_time == 0xF) config->suppress_time = 3;  // 3 seconds

// #ifdef DEBUG_OUTPUT
  // hbwdebug(F("cfg DBPin:"));
  // hbwdebug(pin);
  // hbwdebug(F(" repeat_long_p:"));
  // hbwdebug(config->repeat_long_press);
  // hbwdebug(F(" pullup:"));
  // hbwdebug(config->pullup);
  // hbwdebug(F("\n"));
// #endif
};


uint8_t HBWPhoneDial::get(uint8_t* data)
{
	if (lineState != line_status::idle)  // ... testing. Show hook state as channel on/off. TODO: use state flags for phoneState?
		(*data) = 200;
	else
		(*data) = 0;
  
	return 1;
};


void HBWPhoneDial::loop(HBWDevice* device, uint8_t channel)
{
  phoneLoop(); // handle phone actions

  lineState = checkLineState();  // check the status of the phone line

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

  if (phoneState != phone_status::onHook || lineState != line_status::idle) {
    hbwdebug(F(" - busy. DialDelay: ")); hbwdebug(phoneActionDelay);
    hbwdebug(F(" remaining: ")); hbwdebug(phoneActionDelay - (millis() - phonePreviousMillis));
    hbwdebug(F("\n"));
    return false;  // already dialed or active call
  }
  phoneState = phone_status::dialing;
  dbChannel = chan_calc; // store the channel number by which we got triggered
  setNewPhoneTimer();  // clear timer
  // lineState = line_status::idle;  // TODO: proper reset?
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
          // setNewPhoneTimer(HANGUP_AFTER * 1000);  // TODO: config option max call curation
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
      }
    }
    else {
      // press and hold for configured time
      _buttonPressed = true;
      _dialKeyPressAndPauseDelay = DIAL_PRESS_TIME;
      _n = _numStr[_i] - '0';
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
    case line_status::lineError:
    case line_status::unknown: {  // TODO: should trigger a reset when this state remains for too long?
          if (!lineIsActive()) return line_status::idle;
      }
      break;
    // default:
    //   return line_status::unknown;
  }
  return lineState;  // no change
};
