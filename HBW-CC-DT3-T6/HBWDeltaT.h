/*
 * HBWDeltaT.h
 *
 * Created on: 05.05.2019
 * loetmeister.de
 * 
 * DeltaT channel takes input values from two DeltaTx channels (T1, T2)
 * DeltaT calculates T2 - T1 and set state on/off
 *
 */
 
#ifndef HBWDELTAT_H_
#define HBWDELTAT_H_


#include "HBWired.h"

#define DEBUG_OUTPUT   // debug output on serial/USB


#define OFF LOW
#define ON HIGH

#define MIN_CHANGE_WAIT_TIME 30000 // 30 seconds // TODO: make it part of config?
#define DELTA_CALCULATION_WAIT_TIME 3100 //  3.1 seconds // TODO: make it part of config?


#define DEFAULT_TEMP -27315   // for unused channels
#define ERROR_TEMP -27314     // CRC or read error


// config of one DeltaT channel, address step 5
struct hbw_config_DeltaT {
  uint8_t logging:1;      // +0.0   1=on 0=off
  uint8_t locked:1;     // +0.1   1=LOCKED, 0=UNLOCKED
  uint8_t n_inverted:1;   // +0.2   inverted logic (0=inverted)
  //uint8_t :1;     //fillup
  uint8_t deltaHys:5;   // temperature hysteresis (factor 10), max 30 = 3.0°C (3.0°C for on & off = 6.0°C)
  //uint8_t deltaHys;   // temperature hysteresis (factor 10)
  uint8_t deltaT;   // temperature delta (factor 10), max. 254 = 25.4°C
  int16_t maxT1;   // centi celcius (factor 100)
  int16_t minT2;   // centi celcius (factor 100)
};

// config of one DeltaTx channel, address step 2
struct hbw_config_DeltaTx {
  uint8_t unused:4;
  uint8_t :4;     //fillup
  uint8_t dummy:8;
};


// Class HBWDeltaTx (input channel, to peer with internal or external temperatur sensor)
class HBWDeltaTx : public HBWChannel {
  public:
    HBWDeltaTx(hbw_config_DeltaTx* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
	  virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);
	  virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);  // allow set() only if not peered?
//    virtual void afterReadConfig();

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
    //virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();

  private:
    hbw_config_DeltaT* config;
    uint8_t pin;
    HBWDeltaTx* deltaT1 = NULL;
    HBWDeltaTx* deltaT2 = NULL;

    //uint8_t deltaT;
    int16_t deltaT;
    uint32_t outputChangeLastTime;    // last time output state was changed
    uint32_t deltaCalcLastTime;    // last time of calculation
    // uint32_t outputChangeNextDelay;    // time until next state change
    uint8_t keyPressNum;

    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending

    bool calculateNewState();
    bool setOutput(HBWDevice* device, uint8_t channel);

    bool currentState;
    bool nextState;
    bool initDone;
    

    union tag_state_flags {
      struct state_flags {
        uint8_t notUsed :4; // lowest 4 bit are not used, based on XML state_flag definition
        uint8_t dlimit  :1; // delta within limit (0 = temperature exceeds max delta value)
        uint8_t mode    :1; // true = active (both input values received)
        uint8_t status  :1; // outputs on or off?
      } element;
      uint8_t byte:8;
    } stateFlags;
    
};

#endif /* HBWDELTAT_H_ */
