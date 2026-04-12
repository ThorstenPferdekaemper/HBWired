/*
 * HBWKeyDoorbell.cpp
 *
 * Created (www.loetmeister.de): 25.01.2020
 */

#include "HBWKeyDoorbell.h"

bool HBWPhoneDial_Base::DialNumber(uint8_t) { return 0; };  // dummy function. To be owerwritten by phoneDialChan (HBWPhoneDial)

// Class HBWKeyDoorbell
// standard channel type
HBWKeyDoorbell::HBWKeyDoorbell(uint8_t _pin, hbw_config_key_doorbell* _config, uint8_t _pinBuzzer, bool _activeHigh)
{
  pin = _pin;
  config = _config;
  pinBuzzer = _pinBuzzer;
  activeHigh = _activeHigh;
  keyPressedMillis = 0;
  keyPressNum = 0;
  repeatCounter = 0;
};

// special channel type - to refer HBWPhoneDial channel
HBWKeyDoorbell::HBWKeyDoorbell(uint8_t _pin, hbw_config_key_doorbell* _config, HBWPhoneDial_Base* _phone_dial_chan, uint8_t _pinBuzzer, bool _activeHigh)
{
  keyPressedMillis = 0;
  keyPressNum = 0;
  repeatCounter = 0;
  pin = _pin;
  config = _config;
  phoneDialChan = _phone_dial_chan;
  pinBuzzer = _pinBuzzer;
  activeHigh = _activeHigh;
};


void HBWKeyDoorbell::afterReadConfig()
{
  if(config->long_press_time == 0xFF) config->long_press_time = 10;  // 1.0 second
  if (config->pullup && !activeHigh)  pinMode(pin, INPUT_PULLUP);
  else  pinMode(pin, INPUT);

  if (config->suppress_num == 0xF) config->suppress_num = 3;
  if (config->suppress_time == 0xF) config->suppress_time = 3;  // 3 seconds

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
  if (now == 0) now = 1;  // do not allow time=0 for the below code
  
  bool buttonState = activeHigh ^ ((digitalRead(pin) ^ !config->n_inverted));
  
  if (repeatCounter != 0 && lastSentLong == 0 && (now - lastKeyPressedMillis >= (uint32_t)config->suppress_time *1000)) {
    repeatCounter = 0;  // reset repeat counter when suppress time has passed and no buttons are pressed anymore
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
          if (device->sendKeyEvent(channel, keyPressNum, false) != HBWDevice::BUS_BUSY) // TODO: should add retry?
          {
            repeatCounter = config->suppress_num;
            buzzer(buzzerAction::SUCCESS, true);
            phoneDialChan->DialNumber(channel);
          }
        }
        else {
          repeatCounter--;
          buzzer(buzzerAction::BLOCKED);
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
        if (now - lastSentLong >= (uint32_t)config->suppress_time *1000 && config->repeat_long_press)
        {
          // alle suppress_time wiederholen, wenn repeat_long_press gesetzt
          // keyPressNum nicht erhoehen
          device->sendKeyEvent(channel, keyPressNum, true);  // long press
          lastSentLong = now;
          repeatCounter = config->suppress_num;
          buzzer(buzzerAction::SUCCESS);
        }
      }
      else if (now - keyPressedMillis >= long(config->long_press_time) * 100) {
        if (repeatCounter == 0) {
          // erstes LONG
          keyPressNum++;
          if (device->sendKeyEvent(channel, keyPressNum, true) != HBWDevice::BUS_BUSY)  // long press // TODO: should add retry?
          {
            lastSentLong = now;
            repeatCounter = config->suppress_num;
            buzzer(buzzerAction::SUCCESS);
            phoneDialChan->DialNumber(channel);
          }
        }
      }
    }
    else {
      // Taste war vorher nicht gedrueckt
      keyPressedMillis = now;
      lastKeyPressedMillis = now;
      lastSentLong = 0;
    }
  }
  
  buzzer(buzzerAction::PLAY);
};


void HBWKeyDoorbell::buzzer(uint8_t _action, bool _forceChange)
{
  if (pinBuzzer == NOT_A_PIN)  return;
  if (config->buzzer == 0)  return;

  static unsigned char thisNote, selectedMelody, currentAction;
  static unsigned long previousMillis;
  static unsigned int melodyDelay;

  if (_action != buzzerAction::PLAY && (melodyDelay == 0 || _forceChange))
  {
    // init
    currentAction = _action;
    selectedMelody = MAX_MELODY_CONFIG - config->buzzer;
    melodyDelay = 1;
    thisNote = 0;
  }

  if ((millis() - previousMillis >= melodyDelay) && (melodyDelay > 0 || thisNote != 0))
  {
    previousMillis = millis();

    if (thisNote < NUM_NOTES) {
      // to calculate the note duration, take one second divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      unsigned int noteDuration = pgm_read_word(&melody[selectedMelody][currentAction][NOTE_LEN][thisNote]);
      if (noteDuration > 0) {
        noteDuration = 1000 / noteDuration;
        tone(pinBuzzer, pgm_read_word(&melody[selectedMelody][currentAction][NOTE][thisNote]), noteDuration);
      }
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      melodyDelay = noteDuration * 1.30;
  // hbwdebug(thisNote);hbwdebug(F(",dly "));hbwdebug(melodyDelay);hbwdebug(F("\n"));
      thisNote++;  // iterate over the notes of the melody
    }
    else {
      noTone(pinBuzzer);
      thisNote = 0;
      melodyDelay = 0;
  // hbwdebug(F("melody done\n"));
    }
  }
};

