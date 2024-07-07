// TODO
// Support Bresser 7in1 or 6in1 sensor

#ifndef HBWSIGNALDuino_bresser7in1_h
#define HBWSIGNALDuino_bresser7in1_h

#include <inttypes.h>
#include "HBWired.h"

// extern struct s_hbw_link {
// 	uint8_t msg_ready:1;
// 	uint8_t fillup:7;
// 	uint8_t rssi;
// };

// config of one channel, address step ?
struct hbw_config_signalduino_wds_7in1 {
  uint16_t id;  // sensor ID, to read always same sensor (0xFFFF or 0x0 == auto learn)
  uint8_t type:2; // Bresser 7in1 (default == 3) or 6in1 sensor (TODO other simlar ones?)
  uint8_t fillup:6;
  uint16_t oof;
  uint8_t dummyx;
};

 // TODO? WDS base class + bresser7in1 extension?
//  class HBWSIGNALDuino_bresser7in1 : public HBWSIGNALDuino_wds
class HBWSIGNALDuino_bresser7in1 : public HBWChannel {
  public:
    // HBWSIGNALDuino_bresser7in1(uint8_t* _msg_buffer_ptr, uint8_t* _msg_ready_ptr, hbw_config_signalduino_wds_7in1* _config);
    HBWSIGNALDuino_bresser7in1(uint8_t* _msg_buffer_ptr, uint8_t* _hbw_link, hbw_config_signalduino_wds_7in1* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    // virtual void set(HBWDevice* device, uint8_t length, uint8_t const * const data);
    virtual void afterReadConfig();

  private:
    bool parseMsg();
    int calcRSSI(uint8_t _rssi);
    hbw_config_signalduino_wds_7in1* config;
    uint8_t* msg_buffer_ptr;
    // uint8_t* msg_ready;
    uint8_t* hbw_link;
    uint8_t message_buffer[26];  // need only 25? / ccMaxBuf 50; ccBuf[ccMaxBuf + 2] from cc1101.h
    uint8_t rssi;
    uint32_t lastCheck;
    bool printDebug = false;

    uint8_t const preamble_pattern[5] = {0xaa, 0xaa, 0xaa, 0x2d, 0xd4};
    // char winddirtxtar[] = {'N','NNE','NE','ENE','E','ESE','SE','SSE','S','SSW','SW','WSW','W','WNW','NW','NNW','N'};
    // uint16_t lfsr_digest16(uint8_t const message[], unsigned bytes, uint16_t gen, uint16_t key);
    };

#endif
