/*
 * HBWDeltaT.h
 *
 * Created on: 05.05.2019
 * loetmeister.de
 * 
 * A DeltaT channel takes input temperature from two fixed DeltaTx channels (T1, T2),
 * which are peered with internal or external temperature sensors.
 * DeltaT calculates T2 - T1 and set output state on/off. Hysteresis is subtracted if
 * delta is below threshold or added if above. If DeltaTx channels are above maxT1 or 
 * below minT2, the output is switched off and no delta comparison is performed.
 *
 */
 
#ifndef HBWDELTAT_H_
#define HBWDELTAT_H_


#include "HBWired.h"
#include "HBWOneWireTempSensors.h"


#define DEBUG_OUTPUT   // debug output on serial/USB

// #define DT_ENABLE_PEERING  // define this, to allow DeltaT channel to send key events to external actor. This also requires HBWLinkKeyInfoEventSensor.h for peering. // TODO rework HBWLink... libs to allow combination of different HBWLinkSender classes

#define OFF LOW
#define ON HIGH

static const uint16_t  DELTA_CALCULATION_WAIT_TIME = 3100;  // delta T calculation, every 3.1 seconds (new temperature values should not be received faster than 5 seconds - usually 10 seconds min pause)


// config of one DeltaT channel, address step 7
struct hbw_config_DeltaT {
  uint8_t logging:1;      // +0.0   1=on 0=off
  uint8_t locked:1;     // +0.1   1=LOCKED, 0=UNLOCKED
  uint8_t n_inverted:1;   // +0.2   inverted logic (0=inverted)
  uint8_t deltaHys:5;   // temperature hysteresis (factor 10), max 30 = 3.0°C (3.0°C for on & off = 6.0°C)
  uint8_t deltaT;   // temperature delta (factor 10), max. 254 = 25.4°C
  int16_t maxT1;   // centi celcius (factor 100)
  int16_t minT2;   // centi celcius (factor 100)
  uint8_t output_change_wait_time:3;  // 5 seconds stepping, allowing 5 - 40 seconds (0...7 +1) *5
  uint8_t n_enableHysMaxT1:1;  // apply hysteresis on maxT1, 1=off (default) 0=on
  uint8_t n_enableHysMinT2:1;  // apply hysteresis on minT2, 1=off (default) 0=on
  uint8_t n_enableHysOFF:1;  // apply hysteresis on OFF transition, too. 1=off (default) 0=on
  uint8_t error_state:1;  // set output OFF or ON at error state (defaut OFF)
  uint8_t :1;     //fillup
};

// config of one DeltaTx channel, address step 1
struct hbw_config_DeltaTx {
  uint8_t unused;  // no config right now
  //uint8_t :4;     //fillup
};


// Class HBWDeltaTx (input channel, to peer with internal or external temperatur sensor)
class HBWDeltaTx : public HBWChannel {
  public:
    HBWDeltaTx(hbw_config_DeltaTx* _config);
    // virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);  // allow set() only if not peered?

    int16_t currentTemperature; // temperature in m°C
    
  private:
    hbw_config_DeltaTx* config;
};


// Class HBWDeltaT (output channel, sets output pin or peered external switch)
class HBWDeltaT : public HBWChannel {
  public:
    HBWDeltaT(uint8_t _pin, HBWDeltaTx* _delta_t1, HBWDeltaTx* _delta_t2, hbw_config_DeltaT* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();

  private:
    hbw_config_DeltaT* config;
    uint8_t pin;
    HBWDeltaTx* deltaT1 = NULL;  // linked virtual HBWDeltaTx input channel
    HBWDeltaTx* deltaT2 = NULL;  // linked virtual HBWDeltaTx input channel

    //uint8_t deltaT;
    int16_t deltaT;
    uint32_t outputChangeLastTime;    // last time output state was changed
    uint32_t deltaCalcLastTime;    // last time of calculation
    uint8_t keyPressNum;

    bool calculateNewState();
    bool setOutput(HBWDevice* device, uint8_t channel);

    bool currentState;
    bool nextState;
    bool initDone;
    

    union tag_state_flags {
      struct state_flags {
        uint8_t notUsed :4; // lowest 4 bit are not used, based on XML state_flag definition
        uint8_t dlimit  :1; // delta within display limit (false = temperature exceeds max delta value)
        uint8_t mode    :1; // true = active (both input values received)
        uint8_t status  :1; // outputs on or off?
      } element;
      uint8_t byte:8;
    } stateFlags;
    
};

#endif /* HBWDELTAT_H_ */
