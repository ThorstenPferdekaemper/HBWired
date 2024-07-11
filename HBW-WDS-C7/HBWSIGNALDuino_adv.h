// TODO
// placheholder - chan should allow basic config for SIGNALDuino, like mode, frequency, bWidth, etc.

#ifndef HBWSIGNALDuino_adv_h
#define HBWSIGNALDuino_adv_h

#include <inttypes.h>
#include "HBWired.h"
// #include <SPI.h>


// config of one channel, address step ?
struct hbw_config_signalduino_adv {
  uint8_t logging:1;              // 0x0000
  uint8_t output_unlocked:1;      // 0x07:1    0=LOCKED, 1=UNLOCKED
  uint8_t n_inverted:1;           // 0x07:2    0=inverted, 1=not inverted (device reset will set to 1!)
  uint8_t        :5;              // 0x0000
  uint16_t onTime;
  uint16_t offTime;
  uint8_t dummyx;
};


class HBWSIGNALDuino_adv : public HBWChannel {
  public:
    HBWSIGNALDuino_adv(uint8_t _pin_receive, uint8_t _pin_send, uint8_t _pin_led, hbw_config_signalduino_adv* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void set(HBWDevice* device, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();

  private:
   uint8_t pin_receive, pin_send, pin_led;
//    uint8_t currentState;    // keep track of logical state, not real IO
    hbw_config_signalduino_adv* config;
    uint32_t lastOnOffTime;
};

#endif
