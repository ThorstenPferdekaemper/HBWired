// TODO
// placheholder - chan should allow basic config for SIGNALDuino, like mode, frequency, bWidth, etc.

#ifndef HBWSIGNALDuino_adv_h
#define HBWSIGNALDuino_adv_h

#include <inttypes.h>
#include <HBWired.h>


// config of one channel, address step ? 16 (must be multiple of four)
struct hbw_config_signalduino_adv {
  uint8_t unused;              // +0
  uint8_t rfmode:4;   // rfmodeTesting 0...15 (15 = default)
  uint8_t fillup:4;
  uint16_t cc_bWidth;
  uint16_t cc_freq;   // cc1101_frequency (0...65534 +30000 = 340.00 to 995.34 MHz)
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
	
	// static const unsigned char* Bresser_5in1_u_7in1__B26_N7_8220[] = {'CW0001,0246,0306,042D,05D4,061A,07C0,0800,0D21,0E65,0FE8,1088,114C,1202,1322,14F8,1551,1700,1818,1916,1B43,1C68,1D91,23E9,242A,2500,2611,3D07,3E01,4042,4172,4265,4373,4473,4535,4631,4700'};
	// static const unsigned char* rfmode[] = {Bresser_5in1_u_7in1__B26_N7_8220};
};

#endif
