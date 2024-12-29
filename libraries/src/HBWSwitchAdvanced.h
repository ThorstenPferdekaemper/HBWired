/* 
* HBWSwitchAdvanced.h
*
* Als Alternative zu HBWSwitch & HBWLinkSwitchSimple sind mit
* HBWSwitchAdvanced & HBWLinkSwitchAdvanced folgende Funktionen möglich:
* Peering mit TOGGLE, TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, onTime,
* offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung).
*
* http://loetmeister.de/Elektronik/homematic/
*
* Some code taken from https://github.com/pa-pa/AskSinPP/blob/master/Switch.h
* 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
*
*/

#ifndef HBWSwitchAdvanced_h
#define HBWSwitchAdvanced_h

#include <inttypes.h>
#include "HBWired.h"
#include "HBWlibStateMachine.h"


// #define DEBUG_OUTPUT   // debug output on serial/USB


// HBWLinkSwitchAdvanced peering layout (7 byte):
//           + 6        //  LONG|SHORT_ACTION_TYPE
//           + 7        //  LONG|SHORT_ONDELAY_TIME
//           + 8        //  LONG|SHORT_ON_TIME
//           + 9        //  LONG|SHORT_OFFDELAY_TIME
//           + 10       //  LONG|SHORT_OFF_TIME
//           + 11, 12   //  LONG|SHORT_JT_* table


// TODO: wahrscheinlich ist es besser, bei EEPROM-re-read
//       callbacks fuer die einzelnen Kanaele aufzurufen 
//       und den Kanaelen nur den Anfang "ihres" EEPROMs zu sagen
// config of one channel, address step 2
struct hbw_config_switch {
  uint8_t logging:1;              // 0x0000
  uint8_t output_unlocked:1;      // 0x07:1    0=LOCKED, 1=UNLOCKED
  uint8_t n_inverted:1;           // 0x07:2    0=inverted, 1=not inverted (device reset will set to 1!)
  uint8_t        :5;              // 0x0000
  uint8_t dummy;
};


class HBWSwitchAdvanced : public HBWChannel {
  public:
    HBWSwitchAdvanced(uint8_t _pin, hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();
    
    enum Actions
    {
      INACTIVE = 0,
      JUMP_TO_TARGET,
      TOGGLE_TO_COUNTER,
      TOGGLE_INVERS_TO_COUNTER,
      TOGGLE
    };

    enum TimeModes
    {
      TIME_MODE_MINIMAL = 0,
      TIME_MODE_ABSOLUTE
    };

    struct s_peering_list // structure must match the EEPROM layout!
    {
       uint8_t actionType    :4;
       uint8_t unused   :1;
       uint8_t LongMultiexecute :1;
       uint8_t offTimeMode   :1;
       uint8_t onTimeMode    :1;
       uint8_t onDelayTime;
       uint8_t onTime;
       uint8_t offDelayTime;
       uint8_t offTime;
       // jumpTargets
       // uint8_t jtOnDelay  : 3;
       // uint8_t jtOn       : 3;
       // uint8_t jtOffDelay : 3;
       // uint8_t jtOff      : 3;
       // uint8_t free      : 4;
       
       inline bool isOffTimeAbsolute() const
       {
          return offTimeMode == TIME_MODE_ABSOLUTE;
       }
       inline bool isOnTimeAbsolute() const
       {
          return onTimeMode == TIME_MODE_ABSOLUTE;
       }
       inline bool isOffTimeMinimal() const
       {
          return offTimeMode == TIME_MODE_MINIMAL;
       }
       inline bool isOnTimeMinimal() const
       {
          return onTimeMode == TIME_MODE_MINIMAL;
       }
    };

    struct s_jt_peering_list // structure must match the EEPROM layout!
    {
       // jumpTargets, 2 bytes;
       uint8_t jtOnDelay  : 3;
       uint8_t jtOn       : 3;
       uint8_t jtOffDelay : 3;
       uint8_t jtOff      : 3;
       uint8_t free      : 4;
       // uint8_t currentKeyNum;
       // uint8_t sameLastSender : 1;
       // uint8_t unused      : 7;
    };

    enum States // peering options/values must match the XML values!
    {
       JT_ONDELAY = 0x00,
       JT_ON = 0x01,
       JT_OFFDELAY = 0x02,
       JT_OFF = 0x03,
       JT_NO_JUMP_IGNORE_COMMAND = 0x04,
       UNKNOWN_STATE = 0xFF
    };


  private:
    uint8_t pin;
    virtual bool setOutput(HBWDevice* device, uint8_t newstate);

  protected:
    hbw_config_switch* config;  // channel config, like logging
    uint8_t currentState;
    bool stateTimerRunning;
    bool oldStateTimerRunningState;
    uint32_t stateChangeWaitTime;
    uint32_t lastStateChangeTime;
    s_peering_list stateParamListS = {};//{0}  // values from peering (on/off time, etc.) for later state changes
    s_peering_list* stateParamList = &stateParamListS;  // assign pointer
    // s_peering_list* stateParamList;  // values from peering, storing on/off time, etc.
    uint8_t lastKeyNum;

    /* set required variables - to have consistent startup state */
    inline void init() {
	  // stateParamList = (s_peering_list*) malloc(sizeof(*stateParamList));  // TODO: no need for dynamic allocation...
      stopStateChangeTime();
      oldStateTimerRunningState = false;
      stateChangeWaitTime = 0;
      lastStateChangeTime = 0;
      lastKeyNum = 255;  // key press counter uses only 6 bit, so init value of 255 makes sure first press (count 0) is accepted
      currentState = UNKNOWN_STATE;  // use UNKNOWN_STATE to trigger e.g. port initilization in afterReadConfig()
    };

    // inline bool currentStateIs(uint8_t _value) {
      // return currentState == _value;
    // };

    inline void stopStateChangeTime() {  // clear timer
      stateTimerRunning = false;
    };

    // inline void setLastStateChangeTime(uint32_t _now = millis()) {  // reset timer
      // lastStateChangeTime = _now;
      // stateTimerRunning = true;
    // };

    inline void startNewStateChangeTime(uint32_t _delay, uint32_t _now = millis()) {  // set timer
      stateChangeWaitTime = _delay;
      lastStateChangeTime = _now;
      stateTimerRunning = true;
    };

    uint32_t getRemainingStateChangeTime(uint32_t _now = millis()) { // TODO: was "...) const {"
      if (stateTimerRunning == false)  return 0;
      return (stateChangeWaitTime - (_now - lastStateChangeTime));  // TODO: check if this can roll over - and if that's an issue?
    };

    uint8_t getNextState() const
    {
      switch(currentState) {
        case JT_ONDELAY:  return JT_ON;
        case JT_ON:       return JT_OFFDELAY;
        case JT_OFFDELAY: return JT_OFF;
        case JT_OFF:      return JT_ONDELAY;
        default: break;
      }
      return JT_NO_JUMP_IGNORE_COMMAND;
    };
    
    uint8_t getJumpTarget(uint8_t _state, const s_jt_peering_list* _peerList) const
    {
      switch(_state) {
        case JT_ONDELAY:  return _peerList->jtOnDelay;
        case JT_ON:       return _peerList->jtOn;
        case JT_OFFDELAY: return _peerList->jtOffDelay;
        case JT_OFF:      return _peerList->jtOff;
        default: break;
      }
      return JT_NO_JUMP_IGNORE_COMMAND;
    };
    
    uint32_t getDelayForState(uint8_t _state, const s_peering_list* _peerList) const
    {
      hbwdebug(F(" getDelayForState ")); hbwdebughex(_state); hbwdebug(F(" val:"));
      if( _peerList == NULL ) {
        hbwdebug(F("default!!\n"));
        return getDefaultDelay(_state);
      }
      uint8_t value = 0;
      
      switch(_state) {
        case JT_ONDELAY:  value = _peerList->onDelayTime; break;
        case JT_ON:       value = _peerList->onTime; break;
        case JT_OFFDELAY: value = _peerList->offDelayTime; break;
        case JT_OFF:      value = _peerList->offTime; break;
        default: break;
      }
      hbwdebug(value);hbwdebug(F(" t:")); hbwdebug(convertTime(value)/1000); hbwdebug(F("s\n"));
      return convertTime(value);
    };
    
    uint32_t getDefaultDelay(uint8_t _state) const
    {
      switch(_state) {
        case JT_ON:
        case JT_OFF:
          return DELAY_INFINITE;
        default: break;
      }
      return DELAY_NO;
    };
    
    // void jumpToTarget(HBWDevice* device, const s_peering_list* _peerList)
    void jumpToTarget(HBWDevice* device, const s_peering_list* _peerList, const s_jt_peering_list* _jtPeerList)
    {
      // uint8_t next = getJumpTarget(currentState, _peerList);
      uint8_t next = getJumpTarget(currentState, _jtPeerList);
      if( next != JT_NO_JUMP_IGNORE_COMMAND )
      {
        // get delay for the next state
        uint32_t nextDelay = getDelayForState(next, _peerList);
        // on/off time mode / absolute / minimal
        if( next == currentState && (next == JT_ON || next == JT_OFF) && nextDelay < DELAY_INFINITE)
        {
          bool minimal = (next == JT_ON) ? _peerList->isOnTimeMinimal() : _peerList->isOffTimeMinimal();
          // if new time is mode "minimal" - we jump out if the new delay is shorter
          if( minimal == true ) {
           #ifdef DEBUG_OUTPUT
            hbwdebug(F("jT Min.Delay:")); hbwdebug(nextDelay); hbwdebug(F("\n"));
           #endif
            uint32_t currentDelay = getRemainingStateChangeTime(); // 0 means DELAY_INFINITE
            if( currentDelay == 0 || currentDelay > nextDelay ) {
             #ifdef DEBUG_OUTPUT
              hbwdebug(F("jT Skip Delay!\n"));
             #endif
              return;
            }
          }
        }
        // switch to next
        setState(device, next, nextDelay, _peerList);
      }
    };
    
    void setState(HBWDevice* device, uint8_t next, uint32_t delay, const s_peering_list* _peerList = NULL, uint8_t deep = 0)
    {
      bool stateOk = true;  // default ok, to allow setting new timer for same state
      
      if (_peerList != NULL) {
        // save new values from valid peering event
        memcpy(stateParamList, _peerList, sizeof(*stateParamList));
        hbwdebug(F("sSt saved\n"));
      }
      // check deep to prevent infinite recursion
      if( next != JT_NO_JUMP_IGNORE_COMMAND && deep < 4) {
        // cancel possible running timer
        stopStateChangeTime();
        hbwdebug(F("setS next:")); hbwdebughex(next); hbwdebug(F(" state:")); hbwdebughex(currentState);
        // if state is different
        if (currentState != next) {
          stateOk = switchState(device, next);  // this will update currentState, unless the channel is locked
          hbwdebug(F("->")); hbwdebughex(currentState);
        }
        if (stateOk) {
          if (delay == DELAY_NO) {
            // go immediately to the next state
            next = getNextState();
            delay = getDelayForState(next, _peerList);
            setState(device, next, delay, _peerList, ++deep);
          }
          else if (delay != DELAY_INFINITE) {
            // start new timer
            startNewStateChangeTime(delay);
          }
         // set notify trigger as well, when timer started/stopped, even with no change of the state
         if (oldStateTimerRunningState != stateTimerRunning) {
           setFeedback(device, config->logging);
         }
       }
       oldStateTimerRunningState = stateTimerRunning;
       hbwdebug(F(" sTimer:"));hbwdebug(stateTimerRunning);hbwdebug(F("\n"));
      }
    };
    
    bool switchState(HBWDevice* device, uint8_t newstate)
    {
      if (setOutput(device, newstate) == true) {
        setFeedback(device, config->logging && (newstate == JT_ON || newstate == JT_OFF));
        currentState = newstate;
        return true;
      }
      else {
        setFeedback(device, config->logging);  // always notify if chan is locked, else only for actual state changes
        hbwdebug(F(" locked!"));
      }
      return false;
    };
    
};

#endif
