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
*/

#ifndef HBWDimmerAdvanced_h
#define HBWDimmerAdvanced_h

#include <inttypes.h>
#include "HBWired.h"


#define DEBUG_OUTPUT   // extra debug output on serial/USB

#define RAMP_MIN_STEP_WIDTH 200 // milliseconds (set in 10 ms steps, last digit will be ignored) - default 450ms

// peering/link values must match the XML/EEPROM values!
#define JT_ONDELAY  0x00
#define JT_RAMP_ON  0x01
#define JT_ON       0x02
#define JT_OFFDELAY 0x03
#define JT_RAMP_OFF 0x04
#define JT_OFF      0x05
#define JT_NO_JUMP_IGNORE_COMMAND 0x06

#define ON_TIME_ABSOLUTE    0x0A
#define OFF_TIME_ABSOLUTE   0x0B
#define ON_TIME_MINIMAL     0x0C
#define OFF_TIME_MINIMAL    0x0D
#define UNKNOWN_STATE       0xFF
#define FORCE_STATE_CHANGE  0xFE


// from HBWLinkDimmerAdvanced:
#define NUM_PEER_PARAMS 18
// assign values based on EEPROM layout (XML!) - data array field num.
#define D_POS_actiontype     0
#define D_POS_onDelayTime    1
#define D_POS_onTime         2
#define D_POS_offDelayTime   3
#define D_POS_offTime        4
#define D_POS_jumpTargets0   5
#define D_POS_jumpTargets1   6
#define D_POS_jumpTargets2   7
#define D_POS_offLevel       8
#define D_POS_onMinLevel     9
#define D_POS_onLevel        10
#define D_POS_peerConfigParam  11
#define D_POS_rampOnTime     12
#define D_POS_rampOffTime    13
#define D_POS_dimMinLevel    14
#define D_POS_dimMaxLevel    15
#define D_POS_peerConfigStep 16
#define D_POS_peerConfigOffDtime 17
#define D_POS_peerKeyPressNum    18 // last array element used for keyPressNum

#define ON_LEVEL_USE_OLD_VALUE  202

#define DIM_UP true
#define DIM_DOWN false
#define ON_LEVEL_PRIO_HIGH  0
#define ON_LEVEL_PRIO_LOW   1


// TODO: wahrscheinlich ist es besser, bei EEPROM-re-read
//       callbacks fuer die einzelnen Kanaele aufzurufen 
//       und den Kanaelen nur den Anfang "ihres" EEPROMs zu sagen
struct hbw_config_dim {
  uint8_t logging:1;              // 0x0000
  uint8_t pwm_range:3;            // 1-7 = 40-100%, 0=disabled
  uint8_t voltage_default:1;      // 0-10V (default) or 1-10V mode
  uint8_t        :3;              // 0x0000
  uint8_t dummy;
};


class HBWDimmerAdvanced : public HBWChannel {
  public:
    HBWDimmerAdvanced(uint8_t _pin, hbw_config_dim* _config);
    virtual uint8_t get(uint8_t* data);   
    virtual void loop(HBWDevice*, uint8_t channel);   
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();
  private:
    uint8_t pin;
    hbw_config_dim* config; // logging
    uint8_t currentValue;
    uint8_t oldValue;
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending
    
    void setOutputNoLogging(uint8_t const * const data);
    void setOutput(HBWDevice* device, uint8_t const * const data);
    uint8_t dimUpDown(uint8_t const * const data, boolean dimUp);
    void prepareOnOffRamp(uint8_t rampTime);
    uint8_t getNextState(uint8_t bitshift);
    inline uint32_t convertTime(uint8_t timeValue);
    
    union tag_actiontype {
      struct tag_actiontype_elements {
        uint8_t actionType:4;
        uint8_t dummy:1;
        uint8_t longMultiexecute:1;
        uint8_t offTimeMode:1;
        uint8_t onTimeMode:1;
      } element;
      uint8_t byte:8;
    } peerParamActionType;
//#define BITMASK_ActionType        B00001111
//#define BITMASK_LongMultiexecute  B00010000
    uint8_t onDelayTime;
    uint8_t onTime;
    uint8_t offDelayTime;
    uint8_t offTime;
    union {
      uint32_t DWORD;
      uint8_t jt_hi_low[4];
    } jumpTargets;
    uint8_t onLevel;
    uint8_t onMinLevel;
    uint8_t offLevel;
    union tag_peerConfigParam {
      struct tag_peerConfigParam_elements {
        uint8_t onDelayMode:1;
        uint8_t onLevelPrio:1;
        uint8_t offDelayBlink:1;
        uint8_t dummy:1;
        uint8_t rampStartStep:4;
      } element;
      uint8_t byte:8;
    } peerConfigParam;
    uint8_t rampOnTime;
    uint8_t rampOffTime;
    uint8_t dimMinLevel;
    uint8_t dimMaxLevel;
    union tag_peerConfigStep {
      struct tag_peerConfigStep_elements {
        uint8_t dimStep:4;
        uint8_t offDelayStep:4;
      } element;
      uint8_t byte:8;
    } peerConfigStep;
    union tag_peerConfigOffDtime {
      struct tag_peerConfigOffDtime_elements {
        uint8_t offDelayNewTime:4;
        uint8_t offDelayOldTime:4;
      } element;
      uint8_t byte:8;
    } peerConfigOffDtime;
    boolean stateTimerRunning;
    boolean currentOnLevelPrio;
    uint8_t currentState;
    uint8_t nextState;
    unsigned long stateChangeWaitTime;
    volatile unsigned long lastStateChangeTime;
    uint8_t lastKeyNum;

    volatile uint16_t rampStepCounter;
    uint8_t rampStep;
    boolean offDelayNewTimeActive;
    boolean offDelaySingleStep;
};


/****************************************
 * Setup PWM, to have 4 channels with 122Hz output,
 * to interface with Eltako LUD12-230V (ideally 100HZ @ min. 10V)
 */
inline void setupPwmTimer1(void) {
  // Setup Timer1 for 122.5Hz PWM
  TCCR1A = 0;   // undo the configuration done by...
  TCCR1B = 0;   // ...the Arduino core library
  TCNT1  = 0;   // reset timer
  TCCR1A = _BV(COM1A1)  // non-inverted PWM on ch. A
         | _BV(COM1B1)  // same on ch. B
         | _BV(WGM11);  // mode 10: ph. correct PWM, TOP = ICR1
  TCCR1B = _BV(WGM13)   // ditto
  //| _BV(WGM12) // fast PWM: mode 14, TOP = ICR1
      | _BV(CS12);   // prescaler = 256
  //ICR1 = 312;  // 100Hz //TODO: ? create custom analogWrite() to consider ICR1 as upper limit: e.g. OCR1A = map(val,0,255,0,ICR1);
  ICR1 = 255; // 122.5Hz - this work with default analogWrite() function (which is setting a level between 0 and 255)
}


inline void setupPwmTimer2(void) {
  // Timer 2 @122.5Hz
    TCNT2  = 0;
    TCCR2A = B10100001; // mode 1: ph. correct  PWM, TOP = OxFF
    TCCR2B = B00000110;   // prescaler = 256
}

#endif
