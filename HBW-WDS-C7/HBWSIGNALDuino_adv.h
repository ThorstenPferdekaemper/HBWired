// TODO
// placheholder - chan should allow basic config for SIGNALDuino, like mode, frequency, bWidth, etc.

#ifndef HBWSIGNALDuino_adv_h
#define HBWSIGNALDuino_adv_h

#include <inttypes.h>
#include <HBWired.h>
// #include <SPI.h>


// config of one channel, address step ? 16
struct hbw_config_signalduino_adv {
  // uint8_t unused;              // 0x0000
    // uint8_t free;
  uint16_t oexyz;   // start must be multiple of four
  uint16_t ofxyz;
  uint16_t dummy;
  uint32_t dummyx;
  uint32_t dummyz;
  // TODO allow to set frequency and other options? (use bank0, other bank for other channels?)
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
