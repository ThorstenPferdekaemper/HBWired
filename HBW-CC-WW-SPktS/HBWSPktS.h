/* 
 * Schwingungspaketsteuerung
*
* http://loetmeister.de/Elektronik/homematic/
*
*/

#ifndef HBWSPktS_h
#define HBWSPktS_h


#include "HBWired.h"
#include "HBWDeltaT.h"


#define DEBUG_OUTPUT   // debug output on serial/USB


// global vars
// volatile unsigned char SPktS_currentValue;
// uint8_t SPktS_outputPin;
void setup_timer();

static const byte WellenpaketSchritte = 25; // 500ms / 20ms (20ms = 1/50Hz)


// config of SPktS (dimmer) channel, address step 4
struct hbw_config_dim_spkts {
	uint8_t logging:1;              // send info message on state changes  //TODO: default off?
  uint8_t max_output:4;      // 20 - 100% (0 = disabled)
  uint8_t :3;     //fillup
  uint8_t max_temp;      // 1..254 °C  ( 0 = "special value", NOT_USED)
  uint8_t auto_off;      // 1..254 seconds ( 0 = "special value", NOT_USED)
  uint8_t dummy;
};


class HBWSPktS : public HBWChannel {
  public:
    HBWSPktS(uint8_t* _pin, hbw_config_dim_spkts* _config, HBWDeltaTx* _temp1, volatile unsigned char* _currentValue);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();

  protected:
    uint8_t* pin;
    volatile unsigned char* currentValue;    // keep track of logical state, not real IO
    uint32_t lastSetTime;
    bool initDone;

  private:
    hbw_config_dim_spkts* config;
    HBWDeltaTx* temp1 = NULL;  // linked virtual HBWDeltaTx input channel
    
    union state_flags {
      struct s_state_flags {
        uint8_t notUsed :4; // lowest 4 bit are not used, based on XML state_flag definition
        uint8_t upDown  :2; // dim up = 1 or down = 2
        uint8_t working :1; // true, if working
        uint8_t tMax  :1;   // max temp reached
      } state;
      uint8_t byte:8;
    };

    state_flags stateFlags;
};

// These define's must be placed at the beginning before #include "TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
// #define TIMER_INTERRUPT_DEBUG         0
// #define _TIMERINTERRUPT_LOGLEVEL_     0

// // Timer0 is used for micros(), millis(), delay(), etc and can't be used
// #define USE_TIMER_1     true

// // To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
// #include "TimerInterrupt.h"

// void HBWSPktS::setup_timer()  // call from main setup()
// {
//   cli();//stop interrupts

//   //set timer1 interrupt at 50Hz
//   TCCR1A = 0;// set entire TCCR1A register to 0
//   TCCR1B = 0;// same for TCCR1B
//   TCNT1  = 0;//initialize counter value to 0
//   // set compare match register for 1hz increments
//   OCR1A = 312;// = (16*10^6) / (50*1024) - 1 (must be <65536)
//   // turn on CTC mode
//   TCCR1B |= (1 << WGM12);
//   // Set CS12 and CS10 bits for 1024 prescaler
//   TCCR1B |= (1 << CS12) | (1 << CS10);  
//   // enable timer compare interrupt
//   TIMSK1 |= (1 << OCIE1A);

//   sei();//allow interrupts
// }


// ISR(TIMER1_COMPA_vect)
// {
//   //timer1 interrupt 8kHz

//   static byte WellenpaketCnt = 1;

//   if (SPktS_currentValue && WellenpaketCnt <= SPktS_currentValue) digitalWrite(SPktS_outputPin, true);
//   else  digitalWrite(SPktS_outputPin, false);

//   if (WellenpaketCnt < WellenpaketSchritte) WellenpaketCnt++;
//   else WellenpaketCnt = 1;
// }

// void TimerHandler(unsigned int outputPin)
// {
//   static byte WellenpaketCnt = 1;

//   if (SPktS_currentValue && WellenpaketCnt <= SPktS_currentValue) digitalWrite(outputPin, true);
//     else  digitalWrite(outputPin, false);

//     if (WellenpaketCnt < WellenpaketSchritte) WellenpaketCnt++;
//     else WellenpaketCnt = 1;
// }

// #define TIMER_INTERVAL_MS        20


#endif /* HBWSPktS_h */
