/* 
* HBWlibStateMachine
*
* Support lib for state machine
*
* http://loetmeister.de/Elektronik/homematic/
*
*/

#ifndef HBWlibStateMachine_h
#define HBWlibStateMachine_h

#include <inttypes.h>
#include <Arduino.h>


#define ON_TIME_ABSOLUTE    0x0A
#define OFF_TIME_ABSOLUTE   0x0B
#define ON_TIME_MINIMAL     0x0C
#define OFF_TIME_MINIMAL    0x0D
#define UNKNOWN_STATE       0xFF
#define FORCE_STATE_CHANGE  0xFE


#define ON_LEVEL_PRIO_HIGH  0
#define ON_LEVEL_PRIO_LOW   1

#define BITMASK_ActionType        B00001111
#define BITMASK_LongMultiexecute  B00100000
#define BITMASK_OffTimeMode       B01000000
#define BITMASK_OnTimeMode        B10000000

#define BITMASK_OnDelayMode   B00000001
#define BITMASK_OnLevelPrio   B00000010
#define BITMASK_OffDelayBlink B00000100
#define BITMASK_RampStartStep B11110000

class HBWlibStateMachine {
    public:
    /* convert time value stored in EEPROM to milliseconds - 1 byte value (used for state machine) */
    inline uint32_t convertTime(uint8_t timeValue) {
      
      uint8_t factor = timeValue & 0xC0;    // mask out factor (higest two bits)
      timeValue &= 0x3F;    // keep time value only
    
      // factors: 1,60,1000,6000 (last one is not used)
      switch (factor) {
        case 0:          // x1
          return (uint32_t)timeValue *1000;
          break;
        case 64:         // x60
          return (uint32_t)timeValue *60000;
          break;
        case 128:        // x1000
          return (uint32_t)timeValue *1000000;
          break;
    //    case 192:        // not used value
    //      return 0; // TODO: check how to handle this properly, what does on/off time == 0 mean? infinite on/off??
    //      break;
    //TODO: check; case 255: // not used value?
      }
      return 0;
    };

    /* convert time value stored in EEPROM to milliseconds - 2 byte value (used for state machine) */
    inline uint32_t convertTime(uint16_t timeValue) {
      
      uint8_t factor = timeValue >> 14;    // mask out factor (higest two bits)
      timeValue &= 0x3FFF;    // keep time value only
    
      // factors: 0.1,1,60,1000 (last one is not used)
      switch (factor) {
        case 0:          // x0.1
          return (uint32_t)timeValue *100;
          break;
        case 1:          // x1
          return (uint32_t)timeValue *1000;
          break;
        case 2:         // x60
          return (uint32_t)timeValue *60000;
          break;
    //    case 192:        // not used value
    //      return 0; // TODO: check how to handle this properly, what does on/off time == 0 mean? infinite on/off??
    //      break;
    //TODO: check; case 255: // not used value?
      }
      return 0;
    };

    // /* read jump target entry - set by peering (used for state machine) */
    // inline uint8_t getJumpTarget(uint8_t bitshift) {
      
      // uint8_t nextJump = ((jumpTargets.DWORD >>bitshift) & B00000111);
      
      // if (nextJump == JT_ON) {
        // if (onTime != 0xFF)      // not used is 0xFF
          // nextJump = getOnTimeMode();  // on time ABSOLUTE or MINIMAL?
      // }
      // else if (nextJump == JT_OFF) {
        // if (offTime != 0xFF)      // not used is 0xFF
          // nextJump = getOffTimeMode();
      // }
      // if (stateTimerRunning && nextState == FORCE_STATE_CHANGE) { // timer still running, but update forced // TODO: integrate into above "if". ?Fix: not checking time for 0xFF?
        // if (currentState == JT_ON)
          // nextJump = getOnTimeMode();
          // // TODO: add peerConfigParam.element.onLevelPrio
          // // does HIGH onLevelPrio overwrites ON_TIME_ABSOLUTE and vice versa?
        // else if (currentState == JT_OFF)
          // nextJump = getOffTimeMode();
      // }
      // return nextJump;
    // };
    

    /* read jump target entry - set by peering (used for state machine) */
    /* on & off values could de different per device type (e.g. switch, dimmer) and need to be provided to the function *
    * TODO: check if possible to use global definition in the sketch or device specific definition file with Arduino... */
    inline uint8_t getJumpTarget(uint8_t bitshift, const uint8_t jt_on_value, const uint8_t jt_off_value) {
    // uint8_t getJumpTarget(uint8_t bitshift) {
      
      uint8_t nextJump = ((jumpTargets.DWORD >>bitshift) & B00000111);
      
      if (nextJump == jt_on_value) {
        if (onTime != 0xFF)      // not used is 0xFF
          nextJump = getOnTimeMode();  // on time ABSOLUTE or MINIMAL?
      }
      else if (nextJump == jt_off_value) {
        if (offTime != 0xFF)      // not used is 0xFF
          nextJump = getOffTimeMode();
      }
      if (stateTimerRunning && nextState == FORCE_STATE_CHANGE) { // timer still running, but update forced // TODO: integrate into above "if". ?Fix: not checking time for 0xFF?
        if (currentState == jt_on_value)
          nextJump = getOnTimeMode();
          // TODO: add peerConfigParam.element.onLevelPrio
          // does HIGH onLevelPrio overwrites ON_TIME_ABSOLUTE and vice versa?
        else if (currentState == jt_off_value)
          nextJump = getOffTimeMode();
      }
      return nextJump;
    };
    
	
    /* set required variables - that not get set before used elsewhere */
    inline void init() {
      onTime = 0xFF;
      offTime = 0xFF;
      jumpTargets.DWORD = 0;
      stateTimerRunning = false;
      stateChangeWaitTime = 0;
      lastStateChangeTime = 0;
      lastKeyNum = 255;  // key press counter uses only 6 bit, so init value of 255 makes sure first press (count 0) is accepted
      currentState = UNKNOWN_STATE;
    }
    
	// TODO: replace peer params with struct? (memcpy(&peerParams, data, NUM_PEER_PARAMS)
    union {
      uint32_t DWORD;
      uint8_t jt_hi_low[4];
    } jumpTargets;
    uint8_t onTime;
    uint8_t onDelayTime;
    uint8_t offTime;
    uint8_t offDelayTime;
    uint8_t onLevel;
    uint8_t onMinLevel;
    uint8_t offLevel;
    boolean stateTimerRunning;
    boolean absoluteTimeRunning;
    uint32_t stateChangeWaitTime;
    uint32_t lastStateChangeTime;
    uint8_t lastKeyNum;
    
    inline uint8_t getCurrentState() {
      return currentState;
    }
    inline void setCurrentState(uint8_t value) {
      currentState = value;
    }
    inline boolean currentStateIs(uint8_t value) {
      return currentState == value;
    }
    
    inline uint8_t getNextState() {
      return nextState;
    }
    inline void setNextState(uint8_t value) {
      nextState = value;
    }
    inline void keepCurrentState() {
      nextState = currentState;
    }
    inline boolean noStateChange() {
      return (nextState == currentState);
    }
    inline void forceStateChange() {
      nextState = FORCE_STATE_CHANGE; // force update
    }
    
    inline void writePeerParamActionType(uint8_t value) {
      peerParamActionType = value;
    }
    inline uint8_t getPeerParamActionType() {
      return peerParamActionType;
    }
    inline uint8_t peerParam_getActionType() {
      return peerParamActionType & BITMASK_ActionType;
    }
    inline boolean peerParam_getLongMultiexecute() {
      return peerParamActionType & BITMASK_LongMultiexecute;
    }
    inline uint8_t getOffTimeMode() {
      return (peerParamActionType & BITMASK_OffTimeMode) ? OFF_TIME_ABSOLUTE : OFF_TIME_MINIMAL;  // off time ABSOLUTE or MINIMAL?
    }
    inline uint8_t getOnTimeMode() {
      return (peerParamActionType & BITMASK_OnTimeMode) ? ON_TIME_ABSOLUTE : ON_TIME_MINIMAL;  // on time ABSOLUTE or MINIMAL?
    }
    inline uint8_t peerParam_offTimeMinimal() {
      return (peerParamActionType & BITMASK_OffTimeMode) ? false : true;  // off time MINIMAL?
    }
    inline uint8_t peerParam_onTimeMinimal() {
      return (peerParamActionType & BITMASK_OnTimeMode) ? false : true;  // on time MINIMAL?
    }
    
    inline void writePeerConfigParam(uint8_t value) {
      peerConfigParam = value;
    }
    inline boolean peerParam_onDelayModeSetToOff() {
      return (peerConfigParam & BITMASK_OnDelayMode) == 0;
    }
    inline boolean peerParam_onDelayModeNoChange() {
      return (peerConfigParam & BITMASK_OnDelayMode) == 1;
    }
    inline boolean peerParam_getOnLevelPrio() {
      return peerConfigParam & BITMASK_OnLevelPrio;
    }
    inline boolean peerParam_onLevelPrioIsLow() {
      return (peerConfigParam & BITMASK_OnLevelPrio) == ON_LEVEL_PRIO_LOW;
    }
    inline boolean peerParam_onLevelPrioIsHigh() {
      return (peerConfigParam & BITMASK_OnLevelPrio) == ON_LEVEL_PRIO_HIGH;
    }
    inline boolean peerParam_offDelayBlinkEnabled() {
      return peerConfigParam & BITMASK_OffDelayBlink;
    }
    inline uint8_t peerParam_getRampStartStep() {
      return (peerConfigParam & BITMASK_RampStartStep) >> 4;
    }

    inline void setLastStateChangeTime_now() {
      lastStateChangeTime = millis();
    }

  private:
    uint8_t peerParamActionType;
    uint8_t peerConfigParam;
    uint8_t currentState;
    uint8_t nextState;
    
  protected:
      
};

#endif
