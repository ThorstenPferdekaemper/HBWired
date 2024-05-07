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


#define FIXED_SAMPLE_TIME 1000 // pid compute every second. Removes setSampleTime() from code


// config of one PID channel, address step 9
struct hbw_config_pid {
  uint8_t startMode:1;  // 0x..:1 1=automatic 0=manual
  uint8_t :7;     //fillup //0x..:1-8
  uint16_t kp;    // proportional
  uint16_t ki;    // integral
  uint16_t kd;    // derivative
  uint8_t windowSize;  // 10 seconds steps = max 2540 seconds (windowSize == CYCLE_TIME in XML)
  uint8_t setPoint; // default setPoint (range 0.1 - 25.4°C)
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
    HBWValve* valve {NULL};  // linked valve channel, needs input value of 0-200 (maps to 0-100%)

    bool initDone;
    bool autoTuneRunning; // TODO: implement // 0 = off ; 1 = autotune running
    bool inErrorState;
    bool inAuto; // 1 = automatic ; 0 = manual
    bool oldInAuto; // auto or manual state stored here, when in error pos.
    int16_t setPoint; // temperatures in m°C
    uint32_t windowStartTime;
    
    // pidlib
    int16_t input, lastInput;
   #ifdef FIXED_SAMPLE_TIME
    static const uint16_t sampleTime = FIXED_SAMPLE_TIME;
   #else
    uint32_t sampleTime = 100;  // lib default, 100 ms
   #endif
    uint32_t lastPidTime; // pid computes every sampleTime
    double output, outputSum, outMax;
    double kp, ki, kd;

    void autoTune();
    int32_t mymap(double x, double in_max, double out_max);
    //pid lib
    void compute();
    void setTunings(double Kp, double Ki, double Kd);
    void setSampleTime(int NewSampleTime);
    void setOutputLimits(double Max);
    void setMode(bool Mode);
    void initialize();

    static const bool MANUAL = false;
    static const bool AUTOMATIC = true;
    static const bool SET_BY_PID = true;
    static const int16_t DEFAULT_SETPOINT = 2100;  // 21.00 °C

};


#endif /* HBWPIDS_H_ */
