/* 
* HBWDimmerAdvanced
*
* Mit HBWDimmerAdvanced & HBWLinkDimmerAdvanced sind folgende Funktionen möglich:
* Peering mit TOGGLE_TO_COUNTER, TOGGLE_INVERSE_TO_COUNTER, UPDIM, DOWNDIM,
* TOGGLEDIM, TOGGLEDIM_TO_COUNTER, TOGGLEDIM_INVERSE_TO_COUNTER, onTime,
* offTime (Ein-/Ausschaltdauer), onDelayTime, offDelayTime (Ein-/Ausschaltverzögerung)
* RampOn, RampOff.
*
* http://loetmeister.de/Elektronik/homematic/
*
* Some code taken from https://github.com/pa-pa/AskSinPP/blob/master/Dimmer.h
* 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
*/

#ifndef HBWDimmerAdvanced_h
#define HBWDimmerAdvanced_h

#include <inttypes.h>
#include "HBWlibStateMachine.h"
#include "HBWired.h"

// #define DEBUG_OUTPUT   // extra debug output on serial/USB - turn off for prod use



// from HBWLinkDimmerAdvanced:
// // assign values based on EEPROM layout (XML!) - data array field num.
#define D_POS_actiontype     0
#define D_POS_onDelayTime    1
#define D_POS_onTime         2
#define D_POS_offDelayTime   3
#define D_POS_offTime        4
#define D_POS_offLevel       5
#define D_POS_onMinLevel     6
#define D_POS_onLevel        7
#define D_POS_peerConfigParam  8
#define D_POS_rampOnTime     9
#define D_POS_rampOffTime    10
#define D_POS_dimMinLevel    11
#define D_POS_dimMaxLevel    12
#define D_POS_peerConfigStep 13
#define D_POS_peerConfigOffDtime 14
#define D_POS_jumpTargets0   15
#define D_POS_jumpTargets1   16
#define D_POS_jumpTargets2   17
#define D_POS_peerKeyPressNum    18
#define D_POS_peerSameLastSender    19



#define ON_OLD_LEVEL  201

#define DIM_UP true
#define DIM_DOWN false
#define STATEFLAG_DIM_UP 1
#define STATEFLAG_DIM_DOWN 2


// TODO: wahrscheinlich ist es besser, bei EEPROM-re-read
//       callbacks fuer die einzelnen Kanaele aufzurufen 
//       und den Kanaelen nur den Anfang "ihres" EEPROMs zu sagen
// config of one dimmer channel, address step 2
struct hbw_config_dim {
  uint8_t logging:1;              // send feedback (logging)
  uint8_t pwm_range:3;            // 1-7 = 40-100% max level, 0=disabled
  uint8_t voltage_default:1;      // 0-10V (default) or 1-10V mode
  uint8_t        :3;              // 
  uint8_t dummy;
};


class HBWDimmerAdvanced : public HBWChannel {
  public:
    HBWDimmerAdvanced(uint8_t _pin, hbw_config_dim* _config);
    // HBWDimmerAdvanced(uint8_t _pin, hbw_config_dim* _config, HBWKeyVirtual* _vKey);  // TODO: new variant with virtual key?
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
       UPDIM,
       DOWNDIM,
       TOGGLEDIM,
       TOGGLEDIM_TO_COUNTER,
       TOGGLEDIM_INVERS_TO_COUNTER
    };

    enum TimeModes
    {
      TIME_MODE_MINIMAL = 0,
      TIME_MODE_ABSOLUTE
    };

    enum OnDelayMode
    {
      ON_DELAY_SET_TO_OFF = 0,
      ON_DELAY_NO_CHANGE
    };

    enum OnLevelPrio
    {
      ON_LEVEL_PRIO_HIGH = 0,
      ON_LEVEL_PRIO_LOW
    };

    enum DimmingDirection
    {
      NONE = 0,
      UP,
      DOWN
    };

    static const uint8_t NUM_PEER_PARAMS = 18;  // see HBWLinkDimmerAdvanced
    static const uint8_t PEER_PARAM_JT_START = 15;  // start pos of jumpTargets (see HBWLinkDimmerAdvanced / s_peering_list structure)
    static const uint16_t DELAY_RAMP_DEFAULT = 500;  // 500 ms default ramp time (or 1000?)

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
       uint8_t offLevel;
       uint8_t onMinLevel;
       uint8_t onLevel;
       uint8_t onDelayMode :1;  // ONDELAY_MODE: force off during ON_DELAY or keep current level (TODO: ok to set off as offLevel?)
       uint8_t onLevelPrio :1;  // keep current ON level and onTime with prio high (low prio cannot change these values, only high can) - TODO: confirm?
       uint8_t offDelayBlink :1;
       uint8_t fillup_p :1;
       uint8_t rampStartStep :4;
       uint8_t rampOnTime;
       uint8_t rampOffTime;
       uint8_t dimMinLevel;
       uint8_t dimMaxLevel;
       uint8_t dimStep :4;
       uint8_t offDelayStep :4;
       uint8_t offDelayNewTime :4;  // time with reduced level...
       uint8_t offDelayOldTime :4;  // and previous level
       // // jumpTargets - 3 byte - 'moved' to s_jt_peering_list
       // uint8_t jtOnDelay  : 4;
       // uint8_t jtOn       : 4;
       // uint8_t jtOffDelay : 4;
       // uint8_t jtOff      : 4;
       // uint8_t jtRampOn   : 4;
       // uint8_t jtRampOff  : 4;
       
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
       inline bool isOnLevelPrioHigh() const
       {
          return onLevelPrio == ON_LEVEL_PRIO_HIGH;
       }
       inline bool isOnLevelPrioLow() const
       {
          return onLevelPrio == ON_LEVEL_PRIO_LOW;
       }
       inline bool onDelayModeSetToOff() const
       {
         return onDelayMode == ON_DELAY_SET_TO_OFF;
       }
    };

    struct s_jt_peering_list // keep separate, no need to keep in mem
    {
       // jumpTargets - 3 byte
       uint8_t jtOnDelay  : 4;
       uint8_t jtOn       : 4;
       uint8_t jtOffDelay : 4;
       uint8_t jtOff      : 4;
       uint8_t jtRampOn   : 4;
       uint8_t jtRampOff  : 4;
    };

    enum States // peering options/values must match the XML values!
    {
       JT_ONDELAY = 0x00,
       JT_RAMPON = 0x01,
       JT_ON = 0x02,
       JT_OFFDELAY = 0x03,
       JT_RAMPOFF = 0x04,
       JT_OFF = 0x05,
       JT_NO_JUMP_IGNORE_COMMAND = 0x06,
       TEMP_DIMMING_STATE = 0xFE,
       UNKNOWN_STATE = 0xFF
    };



  private:
    uint8_t pin;  // PWM pin
    uint8_t currentLevel;  // was currentValue
    uint8_t oldOnLevel;  // was oldOnValue
    uint8_t destLevel;
    bool dimmingDirectionUp;  // used to determine direction for state flags, but not only
    
    bool setOutputNoLogging(uint8_t newValue);
    void setOutput(HBWDevice* device, uint8_t newValue);

  protected:
    hbw_config_dim* config; // logging
    uint8_t dimUpDown(s_peering_list* _peeringList, bool dimUp);

    uint8_t currentState;
    bool stateTimerRunning;
    bool oldStateTimerRunningState;
    bool setDestLevelOnce;
    uint32_t changeWaitTime;
    uint32_t lastChangeTime;
    uint16_t offDlyBlinkCounter;
    uint16_t rampStep;
    s_peering_list stateParamListS = {};  // values from peering (on/off time, etc.) for later state changes
    s_peering_list* stateParamList = &stateParamListS;  // assign pointer
    uint8_t lastKeyNum;


    /* set required variables - to have consistent startup state */
    inline void init() {
      stopStateChangeTime();
      oldStateTimerRunningState = false;
      setDestLevelOnce = false;
      offDlyBlinkCounter = 0;
      changeWaitTime = 0;
      lastChangeTime = 0;
      lastKeyNum = 255;  // key press counter uses only 6 bit, so init value of 255 makes sure first press (count 0) is accepted
      currentState = UNKNOWN_STATE;  // use UNKNOWN_STATE to trigger e.g. port initilization in afterReadConfig()
      stateParamList->onLevelPrio = ON_LEVEL_PRIO_LOW;
    };


    inline void stopStateChangeTime() {  // clear timer
      stateTimerRunning = false;
    };

    inline void resetNewStateChangeTime(uint32_t _now = millis()) {  // reset timer
      lastChangeTime = _now;
      stateTimerRunning = true;
    };

    inline void startNewStateChangeTime(uint32_t _delay, uint32_t _now = millis()) {  // set timer
      changeWaitTime = _delay;
      lastChangeTime = _now;
      stateTimerRunning = true;
    };

    uint32_t getRemainingStateChangeTime(uint32_t _now = millis()) {
      if (stateTimerRunning == false)  return DELAY_INFINITE;
      return (changeWaitTime - (_now - lastChangeTime));  // TODO: check if this can roll over - and if that's an issue?
    };

    uint8_t getNextState() const
    {
      switch(currentState) {
        case JT_ONDELAY:  return JT_RAMPON;
        case JT_RAMPON:   return JT_ON;
        case JT_ON:       return JT_OFFDELAY;
        case JT_OFFDELAY: return JT_RAMPOFF;
        case JT_RAMPOFF:  return JT_OFF;
        case JT_OFF:      return JT_ONDELAY;
        default: break;
      }
      return JT_NO_JUMP_IGNORE_COMMAND;
    };
    
    uint8_t getJumpTarget(uint8_t _state, const s_jt_peering_list* _peerList) const
    // uint8_t getJumpTarget(uint8_t _state, const s_peering_list* _peerList) const
    {
      switch(_state) {
        case JT_ONDELAY:  return _peerList->jtOnDelay;
        case JT_RAMPON:   return _peerList->jtRampOn;
        case JT_ON:       return _peerList->jtOn;
        case JT_OFFDELAY: return _peerList->jtOffDelay;
        case JT_RAMPOFF:  return _peerList->jtRampOff;
        case JT_OFF:      return _peerList->jtOff;
        default: break;
      }
      return JT_NO_JUMP_IGNORE_COMMAND;
    };
    
    // uint8_t getLevelForState(uint8_t _state, const s_peering_list* _peerList) const
    // {
      // // hbwdebug(F(" getLevelForState:")); hbwdebughex(_state); hbwdebug(F(" val:"));
      // if( _peerList == NULL ) {
        // // hbwdebug(F("default!!\n"));
        // return (_state == JT_ON || _state == JT_OFFDELAY) ? 200 : 0;
      // }
      // uint8_t value = 0;
      
      // switch(_state) {
        // case JT_ONDELAY:  value = _peerList->onDelayModeSetToOff() ? _peerList->offLevel : currentLevel;
        // // case JT_RAMPON:   value = _peerList->offLevel; break;
        // case JT_ON:       value = _peerList->onLevel; break;
        // case JT_OFFDELAY: value = _peerList->onLevel; break;
        // // case JT_RAMPOFF:  value = _peerList->onLevel; break;
        // case JT_OFF:      value = _peerList->offLevel; break;
        // default: break;
      // }
      // // hbwdebug(value); hbwdebug(F("\n"));
      // return value;
    // };
    
    uint32_t getDelayForState(uint8_t _state, const s_peering_list* _peerList) const
    {
      // hbwdebug(F(" getDelayForState:")); hbwdebughex(_state); hbwdebug(F(" val:"));
      if( _peerList == NULL || _peerList->actionType == 0) {
        // hbwdebug(F("default!!\n"));
        return getDefaultDelay(_state);
      }
      uint8_t value = 0;
      
      switch(_state) {
        case JT_ONDELAY:  value = _peerList->onDelayTime; break;
        case JT_RAMPON:   value = _peerList->rampOnTime; break;
        case JT_ON:       value = _peerList->onTime; break;
        case JT_OFFDELAY: value = _peerList->offDelayTime; break;
        case JT_RAMPOFF:  value = _peerList->rampOffTime; break;
        case JT_OFF:      value = _peerList->offTime; break;
        default: break;
      }
      // hbwdebug(value);hbwdebug(F(" time:")); hbwdebug(convertTime(value)/1000); hbwdebug(F("s\n"));
      return convertTime(value);
    };
    
    uint32_t getDefaultDelay(uint8_t _state) const
    {
      switch(_state) {
        case JT_ON:
        case JT_OFF:
          return DELAY_INFINITE;
        case JT_RAMPON:
        case JT_RAMPOFF:
        return DELAY_RAMP_DEFAULT;
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
            uint32_t currentDelay = getRemainingStateChangeTime();
            if( currentDelay == DELAY_INFINITE || currentDelay > nextDelay ) {
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


    void setState(HBWDevice* device, uint8_t _next, uint32_t _delay, const s_peering_list* _peerList = NULL, uint8_t deep = 0)
    {
      bool stateOk = true;  // default ok, to allow setting new timer for same state
          hbwdebug(F("sSt "));
      
      // check deep to prevent infinite recursion
      if (_next != JT_NO_JUMP_IGNORE_COMMAND && deep < 4) {
        
        if (_peerList != NULL && _peerList->actionType != 0) {
          bool currentOnLevelPrio = stateParamList->onLevelPrio;  // save value before overwriting stateParamList
          
          if (deep == 0) { // add "repeat" bool flag? if (lastKeyNum == currentKeyNum && sameLastSender) flag = true // save not needed for repeated long press
            // currentOnLevelPrio = stateParamList->onLevelPrio;  // save value before overwriting stateParamList
            // save new values from valid peering event
            memcpy(stateParamList, _peerList, sizeof(*stateParamList));
            hbwdebug(F("saved\n"));
          }
        /* pre state change destination-level calculations ------------- */
          // update ON state (ON->ON)
          if (_next == JT_ON && currentState == JT_ON) {
            hbwdebug(F(" ON updt"));
            if (currentOnLevelPrio == ON_LEVEL_PRIO_HIGH && _peerList->isOnLevelPrioLow()) {
              // current prio is high and new is low - keep onLevel and onTime running
              // stateParamList->onLevel = currentLevel; // save level as onLevel TODO: not needed? as we use currentLevel anway?
              destLevel = currentLevel; // keep level
              stateParamList->onLevelPrio = currentOnLevelPrio;  // remains ON_LEVEL_PRIO_HIGH
              _delay = getRemainingStateChangeTime();  // timer restarts (unless INFINITE), continue where we got interrupted
              hbwdebug(F("-Prio "));
            }
            else {
              destLevel = getOnLevel(_peerList);
            }
            setDestLevelOnce = true;  // setOutput(destLevel) in loop once
          }
          // init ON state (anything but ON or Dim ->ON)
          if (_next == JT_ON && currentState != JT_ON && currentState != TEMP_DIMMING_STATE) {
            // set destLevel when going to ON first time, but not during Temp Dim state that calculates own destLevel
            hbwdebug(F(" ON init "));
            destLevel = getOnLevel(_peerList);  // level will be set by state change / switchState below
          }
          // init (...->OFF)
          if (_next == JT_OFF) {
            destLevel = _peerList->offLevel;
            if (currentState == _next) setDestLevelOnce = true;
          }
        } // end (_peerList != NULL && _peerList->actionType != 0)
        
        /* state change ------------- */
        // cancel possible running timer
        stopStateChangeTime();
        offDlyBlinkCounter = 0;
        hbwdebug(F("setS state:")); hbwdebughex(currentState); hbwdebug(F(" next:")); hbwdebughex(_next);
        // if state is different
        if (currentState != _next) {
          stateOk = switchState(device, _next);  // this will update currentState, unless the channel is locked
          hbwdebug(F("->")); hbwdebughex(currentState); hbwdebug(F("\n"));
        }
        
        /* post state change destination-level calculations ------------- */
        if (_peerList != NULL && _peerList->actionType != 0) {
          if ((currentState == JT_ONDELAY || currentState == JT_OFFDELAY) && _delay != DELAY_NO) {
          // update level for on/off delay, prepare off delay blink or step
            if (currentState == JT_ONDELAY) {
              destLevel = _peerList->onDelayModeSetToOff() ? _peerList->offLevel : currentLevel;
            }
            else {  // JT_OFFDELAY
              if (currentLevel > _peerList->onMinLevel) {
                hbwdebug(F("OFFdly "));
                uint8_t offDelayStep = _peerList->offDelayStep *4;
                // don't go below onMinLevel
                rampStep = (int16_t(currentLevel - offDelayStep) > _peerList->onMinLevel) ? offDelayStep : 0;
                
                if (_peerList->offDelayBlink && rampStep) {
                  hbwdebug(F("Blnk "));
                // no blink possible, when rampStep is 0 - but start the off delay
                // TODO: what to do if blink is set, but no offDelayStep? Reduce by e.g. 25%? Or skip level change? (offDlyBlinkCounter = 0)
                // // currentLevel = (currentLevel == blink_level) ? blink_level - (blink_level/4) : blink_level;
                // rampStep = (offDelayStep > 0) ? offDelayStep : 25;
                  destLevel = currentLevel - rampStep;
                  offDlyBlinkCounter = ((convertTime(_peerList->offDelayTime) /1000) / (_peerList->offDelayNewTime + _peerList->offDelayOldTime));
			    // TODO check calculation .... remainder of the off delay time is skipped, if blink times / cycles don't add up
                  _delay = _peerList->offDelayNewTime *1000;  // start with blink newTime (time with reduced level)
                }
                if (_peerList->offDelayStep && !_peerList->offDelayBlink) {
                  hbwdebug(F("Step "));
                  // no blink, but offDelayStep - reduce level by offDelayStep - but not below onMinLevel
                  destLevel = currentLevel - rampStep;
                }
              }
              hbwdebug(F("rStep:"));hbwdebug(rampStep);hbwdebug(F(" "));
            }
            setDestLevelOnce = true; // set level in loop once and allow timer to run
          }
        }
        if (currentState == JT_RAMPON || currentState == JT_RAMPOFF) {
          hbwdebug(F("prep ramp!"));          
          // default vaules, when called with no peering parameters (_peerList)
          uint32_t ramptime = getDelayForState(currentState, _peerList);
          destLevel = (currentState == JT_RAMPOFF) ? 0 : 200;

          if (_peerList != NULL && _peerList->actionType != 0) {  // get ramp parameter from peering
            uint8_t rampStartStep = _peerList->rampStartStep *2;
            
            if (currentState == JT_RAMPON) {
              // destLevel = _peerList->onLevel;
              destLevel = getOnLevel(_peerList);
              if (currentLevel < _peerList->onMinLevel) {  // start at onMinLevel or ramp start step when greater than onMinLevel
                currentLevel = (_peerList->onMinLevel < rampStartStep) ? rampStartStep : _peerList->onMinLevel;
              }
            }
            else { // RAMP OFF
              // destLevel = _peerList->offLevel;
              destLevel = (currentLevel > _peerList->onMinLevel) ? _peerList->onMinLevel : currentLevel;  // dim only to onMinLevel. offLevel will be set at OFF state // offLevel can be higher than onMinLevel... ignore?
              currentLevel -= (int16_t(currentLevel - rampStartStep) > destLevel) ? rampStartStep : 0;  // no start step, if this will go below onMinLevel
            }
              // TODO add a flag to update currentLevel immediately?
          }
          
          // calculate ramp
          uint8_t diff;
          if (currentLevel > destLevel) { // dim down
            diff = currentLevel - destLevel;
          }
          else { // dim up
            diff = destLevel - currentLevel;
          }
          if (diff == 0) {
            // already at the right level... so no ramp needed!
            rampStep = 0; 
            changeWaitTime = DELAY_NO;  // RAMP_MIN_STEP_TIME ? set to 0 or... ?
          } 
          else if( ramptime > diff ) {
            rampStep = 2;//1  // use RAMP_MIN_STEP_WIDTH ? // 2 = 1%
            changeWaitTime = (ramptime / (uint16_t)diff *2);
          }
          else {
            rampStep = uint8_t(diff / (ramptime > 0 ? ramptime : 1));
            changeWaitTime = 10;  // use RAMP_MIN_STEP_TIME ?
          }
          // apply current level, in case it was changed during ramp calculation
          if (setOutputNoLogging(currentLevel))  resetNewStateChangeTime();
          
          hbwdebug(F("rStep:")); hbwdebug(rampStep); hbwdebug(F(" rSTime:")); hbwdebug(changeWaitTime);
          hbwdebug(F(" level:")); hbwdebug(currentLevel); hbwdebug(F(">>")); hbwdebug(destLevel);// hbwdebug(F("\n"));
        } // end (currentState == JT_RAMPON || currentState == JT_RAMPOFF)
        else {
          if (stateOk) {
            if (_delay == DELAY_NO) {
              // go immediately to the next state
              _next = getNextState();
              _delay = getDelayForState(_next, _peerList);
              setState(device, _next, _delay, _peerList, ++deep);
            }
            else if (_delay != DELAY_INFINITE) {
              // start new timer
              startNewStateChangeTime(_delay);
            }
            // set notify trigger as well, when timer started/stopped, even with no change of the state
            if (oldStateTimerRunningState != stateTimerRunning) {
              setFeedback(device, config->logging);  hbwdebug(F(" sf-"));
            }
          }
        }
       if (deep == 0) oldStateTimerRunningState = stateTimerRunning;  // only for final result (ignore recursive temp changes)
       hbwdebug(F(" deep:")); hbwdebug(deep); hbwdebug(F(" sTimer:")); hbwdebug(stateTimerRunning); hbwdebug(F(" delay:")); hbwdebug(_delay);
       hbwdebug(F(" offDlyBlinkCounter:")); hbwdebug(offDlyBlinkCounter); hbwdebug(F(" destLevel:")); hbwdebug(destLevel); hbwdebug(F("\n"));
      }
    };


    bool switchState(HBWDevice* device, uint8_t _next) {
      if (currentState != _next) {
        hbwdebug(F(" swS "));
        if (currentState == JT_ON && (_next == JT_OFF || _next == JT_OFFDELAY || _next == JT_RAMPOFF)) {  // update when going from ON directly to OFF/RAMPOFF/OFFDELAY
          // hbwdebug(F(" oldOnLvl:")); hbwdebug(oldOnLevel);hbwdebug(F(" cLvl:")); hbwdebug(currentLevel);
          oldOnLevel = currentLevel;
        }
        if (setOutputNoLogging(destLevel)) {  // handle disabled channel
          currentState = _next;
          // set_vKey(bool(currentLevel ? true : false));  // TODO: set virtual key status?
        }
        else {
          hbwdebug(F(" !disabled!\n"));
          return false;
        }
        if ( currentState == JT_ON || currentState == JT_OFF ) {
          setFeedback(device, config->logging);
        }
      }
      return true;
    };


    uint8_t getOnLevel(const s_peering_list* _peerList = NULL) {
      // return onLevel or oldOnLevel. Don't use oldOnLevel if below new onMinLevel
      if (_peerList != NULL) {
        return (_peerList->onLevel == ON_OLD_LEVEL) ? ((oldOnLevel < _peerList->onMinLevel) ? _peerList->onMinLevel : oldOnLevel) : _peerList->onLevel;
      }
      return 200;  // full on level as fallback
    };
      
    
    union state_flags {
      struct s_state_flags {
        uint8_t notUsed :4; // lowest 4 bit are not used, based on XML state_flag definition
        uint8_t upDown  :2; // dim up = 1 or down = 2
        uint8_t working :1; // true, if working
        uint8_t status  :1; // outputs on or off? - not used
      } state;
      uint8_t byte:8;
    };
    
};

  // void set_vKey(bool _state) {if (vKey != NULL) vKey->set(_state);}  // TODO: set virtual key status?

#endif
