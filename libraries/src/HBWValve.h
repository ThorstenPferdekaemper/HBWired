/*
 * HBWValve.h
 *
 * Created on: 05.05.2019
 * loetmeister.de
 *
 *
 *  15.02.2015
 *      Author: hglaser
 * Eine einfache Ventilsteuerung für mein Thermisches 24V Ventil.
 * Verwendet wird zur Umrechnung der Ventilstellung die von FHEM gesendet wird,
 * "time proportioning control" eine Art extrem langsames PWM. Bei über 50% schaltet
 * das Ventil zuerst ein, unter 50% zuerst aus. Nach einer Änderung wird die erste
 * Ein- oder Auszeit einmal halbiert
 *
 *
 *25 % ____|----|________|----|_______|
 *     aus  ein  aus      ein  aus...
 *50 % ____|--------|________|--------|____
 *     aus  ein      aus      ein      aus...
 *75 % ----|____|--------|____|--------
 *     ein  aus   ein      aus  ein...
 *
 */
 
#ifndef HBWVAVLE_H_
#define HBWVAVLE_H_


#include "HBWired.h"

#define DEBUG_OUTPUT   // debug output on serial/USB


#define OFF LOW
#define ON HIGH
#define VENTON true
#define VENTOFF false


// need to match frame definition in XML:
#define SET_TOGGLE_AUTOMATIC  201
#define SET_MANUAL  203
#define SET_AUTOMATIC 205


// config of one valve channel, address step 4
struct hbw_config_valve {
  uint8_t logging:1;      // +0.0   1=on 0=off
  uint8_t unlocked:1;     // +0.1   0=LOCKED, 1=UNLOCKED; locked channels will retain level/error_pos. Set error_pos to 0 to disable a channel completely. A locked channel can still be controlled by its PID channel. If PIDs are used, set them to manual start up.
  uint8_t n_inverted:1;   // +0.2   inverted logic (use NO valves, NC is default)
  uint8_t :5;     //fillup //0x..:3-8
  uint8_t error_pos;  // default/error position
  uint8_t valveSwitchTime;   // (factor 10! max 2540 seconds) Time the valve needs to reach 100% (NC:open or NO:closed state)
  uint8_t dummy :8;
  // TODO: option for anti stick? valve_protect (e.g. open valves once a week?)
};


// Class HBWVavle
class HBWValve : public HBWChannel {
  public:
    HBWValve(uint8_t _pin, hbw_config_valve* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data, bool setByPID);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();

    // linked PID channel access
    bool getPidsInAuto();
    void setPidsInAuto(bool newAuto);

  private:
    hbw_config_valve* config;
    uint8_t pin;

    uint8_t valveLevel;
    void setNewLevel(HBWDevice* device, uint8_t NewLevel);
    
    // output control
    inline void switchstate(bool State);
    uint16_t set_timer(bool firstState, bool status);
    uint16_t set_peakmiddle(uint16_t ontimer, uint16_t offtimer);
    bool first_on_or_off(uint16_t ontimer, uint16_t offtimer);
    bool init_new_state();
    uint16_t set_ontimer(uint8_t VentPositionRequested);
    uint16_t set_offtimer(uint16_t ontimer);
    
    uint32_t outputChangeLastTime;    // last time output state was changed
    uint16_t outputChangeNextDelay;    // time until next state change
    uint16_t onTimer, offTimer;     // current calculated on and of duration

    bool initDone;
    bool isFirstState;
    bool nextState;

    union tag_state_flags {  // state_flags should not exeed one byte!
      struct state_flags {
        uint8_t notUsed :4; // lowest 4 bit are not used, based on XML state_flag definition
        uint8_t upDown  :1; // Pid regelt hoch oder runter
        uint8_t inAuto  :1; // 1 = automatic ; 0 = manual
        uint8_t status  :1; // outputs on or off?
        //uint8_t fillup  :1; // not used
      } element;
      uint8_t byte:8;
    } stateFlags;

    static const uint16_t OUTPUT_STARTUP_DELAY = 63;  // 63 == 6.3 seconds
    static const bool MANUAL = false;
    static const bool AUTOMATIC = true;
};

#endif /* HBWVAVLE_H_ */
