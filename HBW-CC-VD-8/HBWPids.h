/*
 * HBWPids.h
 *
 * Created on: 15.04.2019
 * loetmeister.de
 * 
 * Author: Harald Glaser
 * some parts from the Arduino PID Library - Version 1.1.1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 */

#ifndef HBWPIDS_H_
#define HBWPIDS_H_


#include "HBWOneWireTempSensors.h"
#include "HBWired.h"
#include "HBWValve.h"

#define DEBUG_OUTPUT   // debug output on serial/USB


#define MANUAL 0
#define AUTOMATIC 1

// config of one PID channel, address step 9
struct hbw_config_pid {
  uint8_t startMode:1;  // 0x32:1 1=automatic 0=manual
  uint8_t :7;     //fillup //0x32:1-8
  uint16_t kp;    // proportional
  uint16_t ki;    // integral
  uint16_t kd;    // derivative
  uint16_t windowSize;  // TODO: reduce to 1byte? (10 seconds steps? = max 2550 seconds)
};


// Class HBWPids
class HBWPids : public HBWChannel {
  public:
    HBWPids(HBWValve* _valve, hbw_config_pid* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();
    
  private:
    hbw_config_pid* config;
    HBWValve* valve {NULL};  // linked valve channel

    uint8_t desiredValveLevel;

    struct pid_config {
      uint8_t inAuto  :1; // 1 = automatic ; 0 = manual
      uint8_t oldInAuto :1; // auto or manual stored here, when in error pos.
      uint8_t autoTune  :1; // Todo 0 = off ; 1 = autotune running
      uint8_t error :1;
      uint8_t initDone :1;
      int16_t setPoint; // temperatures in m°C
      uint32_t windowStartTime;
      
      // pidlib
      uint32_t outMax;
      double ITerm;
      int16_t Input, lastInput;
      uint16_t sampleTime;
      uint32_t lastPidTime; // pid computes every sampleTime
      double Output;
      double kp, ki, kd;
    } pidConf;

    void autoTune();
    int32_t mymap(double x, double in_max, double out_max);
    int8_t computePid(uint8_t channel);
    //pid lib
    void compute();
    void setTunings(double Kp, double Ki, double Kd);
    void setSampleTime(int NewSampleTime);
    void setOutputLimits(uint32_t Max);
    void setMode(bool Mode);
    void initialize();
};


#endif /* HBWPIDS_H_ */
