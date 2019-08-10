/*
 * HBWSenEP.h
 *
 *  Created on: 30.04.2019
 *      Author: loetmeister.de
 *      Vorlage: Markus, Thorsten
 */

#ifndef HBWSenEP_h
#define HBWSenEP_h


#include "HBWired.h"


//#define DEBUG_OUTPUT   // extra debug output on serial/USB

#define POLLING_TIME 10	  // S0-Puls muss laut Definition mindestens 30ms lang sein - Abtastung mit 10ms sollte ausreichen
                          // POLLING_TIME wird als Standard genommen, kann aber von config polling_time überschrieben werden


// config of each sensor, address step 9
struct hbw_config_sen_ep {
  uint8_t enabled :1;             // 1=enabled, 0=disabled
  uint8_t n_inverted :1;          // 0=inverted, 1=not inverted (device reset will set to 1!)
  uint8_t polling_time :4;        // polling time in ms *10
  uint8_t dummy :2;
  uint16_t send_delta_count;      // Zählerdifferenz, ab der gesendet wird
  uint16_t send_min_interval;     // Minimum-Sendeintervall
  uint16_t send_max_interval;     // Maximum-Sendeintervall
  uint16_t unused;
};


// Class HBWSenEP
class HBWSenEP : public HBWChannel {
  public:
    HBWSenEP(uint8_t _pin, hbw_config_sen_ep* _config, boolean _activeHigh = false);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void afterReadConfig();

  private:
    hbw_config_sen_ep* config;
    uint8_t pin;

    boolean activeHigh;    // activeHigh=true -> input active high, else active low
    boolean currentPortState;
    boolean oldPortState;
    uint16_t currentCount;
    uint16_t lastSentCount;	  // counter on last send
    uint32_t lastSentTime;		// time of last send
    uint32_t lastPortReadTime;		// time of last port read


    inline boolean readInput(uint8_t Pin) {
      boolean reading  = (digitalRead(Pin) ^ config->n_inverted);
      return (activeHigh ^ reading);
    }
};


#endif
