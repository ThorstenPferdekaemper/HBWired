// TODO
// Support Bresser 7in1 or 5in1 sensor
/*
Artikelnummer:   7803200
https://www.bresser.de/Wetter-Zeit/BRESSER-7-in-1-Aussensensor-fuer-7003200-4-Tage-4CAST-WLAN-Wetterstation.html

Artikelnummer:   7803300
https://www.bresser.de/Wetter-Zeit/Zubehoer-bresser-1/Sensoren/BRESSER-7-in-1-Aussensensor-fuer-7003300-WLAN-Comfort-Wetterstation.html
*/

#ifndef HBWSIGNALDuino_bresser7in1_h
#define HBWSIGNALDuino_bresser7in1_h

#include <inttypes.h>
#include "HBWired.h"

#define HBW_CHANNEL_DEBUG

// extern struct s_hbw_link {
// 	uint8_t msg_ready:1;
// 	uint8_t fillup:7;
// 	uint8_t rssi;
// };

// config of one channel, address step ?
struct hbw_config_signalduino_wds_7in1 {
  uint16_t id;  // sensor ID, to read always same sensor (0xFFFF or 0x0 == auto learn)
  uint8_t type:2; // Bresser 7in1 (default == 3) or 5in1 sensor (TODO other simlar ones?)
  uint8_t fillup:6;
  uint16_t oof;
  uint8_t dummyx;
  // TODO allow to set frequency and other options? (use bank0, other bank for other channels?)
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
    // int calcRSSI(uint8_t _rssi);
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

// calculated RSSI and RSSI value
// https://github.com/Ralf9/SIGNALduinoAdv_FHEM/blob/master/FHEM/00_SIGNALduinoAdv.pm
inline int HBWSIGNALDuino_calcRSSI(uint8_t _rssi) {
  return (_rssi>=128 ? ((_rssi-256)/2-74) : (_rssi/2-74));
}
#endif
