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
#include "HBWlibStateMachine.h"
#include "HBWired.h"

#define DEBUG_OUTPUT   // extra debug output on serial/USB - turn off for prod use

#define RAMP_MIN_STEP_WIDTH 250//160 // milliseconds (set in 10 ms steps, last digit will be ignored) - default 250ms


// peering/link values must match the XML/EEPROM values!
#define JT_ONDELAY  0x00
#define JT_RAMP_ON  0x01
#define JT_ON       0x02
#define JT_OFFDELAY 0x03
#define JT_RAMP_OFF 0x04
#define JT_OFF      0x05
#define JT_NO_JUMP_IGNORE_COMMAND 0x06

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
#define D_POS_peerKeyPressNum    18 // last array element always used for keyPressNum

#define BITMASK_DimStep       B00001111
#define BITMASK_OffDelayStep  B11110000

#define BITMASK_OffDelayNewTime B00001111
#define BITMASK_OffDelayOldTime B11110000

#define ON_LEVEL_USE_OLD_VALUE  201

#define DIM_UP true
#define DIM_DOWN false
#define STATEFLAG_DIM_UP 1
#define STATEFLAG_DIM_DOWN 2


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

    enum Actions
    {
       INACTIVE,
       JUMP_TO_TARGET,
       TOGGLE_TO_COUNTER,
       TOGGLE_INVERS_TO_COUNTER,
       UPDIM,
       DOWNDIM,
       TOGGLEDIM,
       TOGGLEDIM_TO_COUNTER,
       TOGGLEDIM_INVERS_TO_COUNTER
    };


  private:
    uint8_t pin;
    hbw_config_dim* config; // logging
    uint8_t currentValue;
    uint8_t oldOnValue;
    uint8_t oldValue;  // used to determine direction for state flags
    HBWlibStateMachine StateMachine;
    
    void setOutputNoLogging(uint8_t newValue);
    void setOutput(HBWDevice* device, uint8_t newValue);
    uint8_t dimUpDown(uint8_t const * const data, boolean dimUp);
    void prepareOnOffRamp(uint8_t rampTime, uint8_t level);
    
    uint8_t rampOnTime;
    uint8_t rampOffTime;
    uint8_t dimMinLevel;
    uint8_t dimMaxLevel;
    boolean currentOnLevelPrio;
    uint16_t rampStepCounter;
    uint16_t rampStep;
    boolean offDelayNewTimeActive;
    boolean offDelaySingleStep;
    uint8_t peerConfigStep;
    uint8_t peerConfigOffDtime;
    
    inline void writePeerConfigStep(uint8_t value) {
      peerConfigStep = value;
    }
    inline uint8_t peerParam_getDimStep() {
      return peerConfigStep & BITMASK_DimStep;
    }
    inline uint8_t peerParam_getOffDelayStep() {
      return ((peerConfigStep & BITMASK_OffDelayStep) >> 4) *4;
    }
    inline void writePeerConfigOffDtime(uint8_t value) {
      peerConfigOffDtime = value;
    }
    inline uint8_t peerParam_getOffDelayNewTime() {
      return peerConfigOffDtime & BITMASK_OffDelayNewTime;
    }
    inline uint8_t peerParam_getOffDelayOldTime() {
      return (peerConfigOffDtime & BITMASK_OffDelayOldTime) >> 4;
    }
    
    union state_flags {
      struct s_state_flags {
        uint8_t notUsed :4; // lowest 4 bit are not used, based on XML state_flag definition
        uint8_t upDown  :2; // dim up = 1 or down = 2
        uint8_t working :1; // true, if working
        uint8_t status  :1; // outputs on or off?
      } element;
      uint8_t byte:8;
    };

  protected:
  
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
