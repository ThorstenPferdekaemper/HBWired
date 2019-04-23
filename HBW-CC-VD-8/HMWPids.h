/*
 * HMWPids.h
 *
 * Created on: 15.04.2019
 * loetmeister.de
 * 
 * Author: Harald Glaser
 * some parts from the Arduino PID Library - Version 1.1.1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 */

#ifndef HMWPIDS_H_
#define HMWPIDS_H_


#include "HBWOneWireTempSensors.h"
#include "HBWired.h"


#define DEBUG_OUTPUT   // debug output on serial/USB

#define MANUAL 0
#define AUTOMATIC 1
#define MAPSIZE 65535.0

//  /* send this to the main Programm */
//  struct pidRetState {
//    int8_t state;
//    uint16_t valveStatus;
//  };

struct hbw_config_pid {
  uint8_t logging:1;    // 0x32:0 1=on 0=off
  uint8_t startMode:1;  // 0x32:1 1=automatic 0=manual
  uint8_t :6;     //fillup //0x32:2-8
  uint16_t kp;    // proportional
  uint16_t ki;    // integral
  uint16_t kd;    // derivative
  uint16_t windowSize;
};


// config of one valve
struct hbw_config_pid_valve {
  uint16_t send_max_interval;   // Maximum-Sendeintervall
  uint8_t error_pos;
};


// Class HBWPids
class HBWPids : public HBWChannel {
  public:
    HBWPids(hbw_config_pid_valve* _configValve, hbw_config_pid* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void setInfo(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();
    
    // pubplic, to get called my PidValve channels
    void setPidsValve(uint8_t const * const data);
    uint8_t getPidsValve(uint8_t* data);
    bool getPidsValveStatus();

  private:
    hbw_config_pid* config;
    hbw_config_pid_valve* configValve;
    //HBWPidsValve* pidValve;

    struct pid_config {
//      uint8_t counter  :6; // is only 6 bits long 0-63
      uint8_t inAuto  :1; // 1 = automatic ; 0 = manual
      uint8_t oldInAuto :1; // auto or manual stored here, when in error pos.
      uint8_t upDown  :1; // Pid regelt hoch oder runter
      uint8_t autoTune  :1; // Todo 0 = off ; 1 = autotune running
      uint8_t status  :1; //bit ??
      uint8_t error :1;
      int16_t setPoint; // temperatures in m°C
      uint16_t outputMap; // pid output mapped to 0 - MAPSIZE (60000)
      uint32_t windowStartTime; // long needed ?? maybe *100
      
      // pidlib
      uint32_t outMax;
      double ITerm;
      int16_t Input, lastInput;
      uint16_t sampleTime; //2 sec
      uint32_t lastPidTime; // pid computes every sampleTime
      double Output;
      double kp, ki, kd;
    } pidConf;

    void autoTune();
    long mymap(double x, double in_max, double out_max);
    int8_t computePid(uint8_t channel);
    void setErrorPosition();
    //pid lib
    void compute();
    void setTunings(double Kp, double Ki, double Kd);
    void setSampleTime(int NewSampleTime);
    void setOutputLimits(uint32_t Max);
    void setMode(bool Mode);
    void initialize();
};


// Class HBWPidsVavle
class HBWPidsValve : public HBWChannel {
  public:
    HBWPidsValve(uint8_t _pin, HBWPids* _pid, hbw_config_pid_valve* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();

  private:
    hbw_config_pid_valve* config;
    uint8_t pin;
    HBWPids* pid;

    //uint8_t Output;
    struct pid_config {
      uint8_t status  :1;
      uint32_t lastSentTime;   // time of last send
    } pidConf;
};


#endif /* HMWPIDS_H_ */
