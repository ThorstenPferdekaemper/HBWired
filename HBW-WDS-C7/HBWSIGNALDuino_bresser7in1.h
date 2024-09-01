/*
* Homematic Wired Hombrew (HBW) channel, with SIGNALDuino
* to Support Bresser 7in1 [or 5in1 sensor - TODO].
* This lib reads the cc1101 receiver message and makes the values available as wather channel.
*
* Artikelnummer:   7803200
* https://www.bresser.de/Wetter-Zeit/BRESSER-7-in-1-Aussensensor-fuer-7003200-4-Tage-4CAST-WLAN-Wetterstation.html
*
* Artikelnummer:   7803300
* https://www.bresser.de/Wetter-Zeit/Zubehoer-bresser-1/Sensoren/BRESSER-7-in-1-Aussensensor-fuer-7003300-WLAN-Comfort-Wetterstation.html
*/

#ifndef HBWSIGNALDuino_bresser7in1_h
#define HBWSIGNALDuino_bresser7in1_h

#include <inttypes.h>
#include <HBWired.h>
#include <HBWOneWireTempSensors.h>

// #define HBW_CHANNEL_DEBUG


static const byte WDS_7IN1_AVG_SAMPLES = 3;  // calculate average of last 3 samples for: temperatue, humidity

// array positions for extern struct s_hbw_link:
enum hbw_link_pos {
  MSG_COUNTER = 0,
  RSSI
};

// config of one channel, address step 16
// for Raspberry Pi Pico: for 16bit types start must be at multiple of four!
struct hbw_config_signalduino_wds_7in1 {
  uint16_t id;  // sensor ID, to read always same sensor (0xFFFF or 0x0 == auto learn)
  uint8_t type:2; // Bresser 7in1 (default == 3) or 5in1 sensor (TODO other simlar ones? -- auto-learn? Or must be set manually?)
  uint8_t average_samples:1;            // TODO: ? average last 3 samples for: temperatue, humidity
  uint8_t fillup:5;
  uint8_t storm_threshold_level:5;        // factor 5: 5...150 km/h
  uint8_t storm_readings_trigger:3;       // storm_threshold readings in a row to trigger storm status (0...7)
  uint8_t send_delta_temp;                // Temperaturdifferenz, ab der gesendet wird (eher nützlich bei goßem Sendeintervall)
  uint8_t timeout_rx:5;                // set timeout after x seconds, 15...450 seconds (0 = disabled). (factor 15: sensor TX interval) 
  uint8_t fillup1:3;
  uint8_t dummy2;  // not in use now...
  uint8_t dummy3;
  uint16_t send_min_interval;            // Minimum-Sendeintervall
  uint16_t send_max_interval;            // Maximum-Sendeintervall
  uint32_t dummy4;  // not in use now...
};

 // TODO? WDS base class + bresser7in1 child?
//  class HBWSIGNALDuino_bresser7in1 : public HBWSIGNALDuino_wds
class HBWSIGNALDuino_bresser7in1 : public HBWChannel {
  public:
    HBWSIGNALDuino_bresser7in1(uint8_t* _msg_buffer_ptr, uint8_t* _hbw_link, hbw_config_signalduino_wds_7in1* _config, uint16_t _eeprom_address_start);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
    virtual void afterReadConfig();

  private:
    uint8_t parseMsg();
    uint8_t get_temp(uint8_t* data);
    hbw_config_signalduino_wds_7in1* config;
    uint16_t eeprom_address_start;  // start address of config in EEPROM
    uint8_t* msg_buffer_ptr;  // SIGNALDuino / cc1101 raw message
    uint8_t* hbw_link;  // from SIGNALDuino, array with etra readgins {new msg flag, rssi}
    uint8_t message_buffer[26];  // need only 25? / ccMaxBuf 50; ccBuf[ccMaxBuf + 2] from cc1101.h
    uint8_t msgCounter;
    // current readings
    uint16_t lightLuxDeci, windDir, windMaxMsRaw, windAvgMsRaw;
    uint32_t rainMmRaw;
    int16_t currentTemp; // temperature in centi celsius
    uint8_t humidityPct;
    // int16_t currentTemp[WDS_7IN1_AVG_SAMPLES]; // temperature in centi celsius
    // uint8_t humidityPct[WDS_7IN1_AVG_SAMPLES];
    uint8_t avgSampleIdx, avgSamples;
    uint8_t uvIndex;
    bool batteryOk;
    int16_t rssi_raw;
    int8_t rssiDb;
    // previous readings, other states
    bool stormy, lastStormy;
    bool msgTimeout;
    uint8_t stormyTriggerCounter;
    int16_t lastSentTemp;
    uint32_t lastCheck, lastSentTime, lastMsgTime;

    union u_state_and_wdir {
      struct s_fields {
        uint8_t windDir :5;
        uint8_t battOk :1;
        uint8_t timeout :1;
        uint8_t free :1;
      } field;
      uint8_t byte:8;
    };

    union u_wind_speed {
      struct s_fields {
        uint16_t windMaxMs :10;
        uint16_t windAvgMs :10;
        uint8_t free :3;
        uint8_t storm :1;
      } field;
      uint8_t first_byte:8;
      uint8_t secnd_byte:8;
      uint8_t third_byte:8;
    };
    
    bool printDebug = false;

    enum msg_parser_ret_code {
      MSG_IGNORED = 0,
      SUCCESS,
      ID_MISSMATCH,
      DECODE_FAIL_MIC
    };

    // uint8_t const preamble_pattern[5] = {0xaa, 0xaa, 0xaa, 0x2d, 0xd4};
    // char winddirtxt[] = {'N','NNE','NE','ENE','E','ESE','SE','SSE','S','SSW','SW','WSW','W','WNW','NW','NNW','N'};
    };


// calculated RSSI and RSSI value
// https://github.com/Ralf9/SIGNALduinoAdv_FHEM/blob/master/FHEM/00_SIGNALduinoAdv.pm
inline int HBWSIGNALDuino_calcRSSI(uint8_t _rssi) {
  return (_rssi>=128 ? ((_rssi-256)/2-74) : (_rssi/2-74));
}

#endif

/* Message layout, 7in1 sensor:
# 0CF0A6F5B98A10AAAAAAAAAAAAAABABC3EAABBFCAAAAAAAAAA000000   original message
# A65A0C5F1320BA000000000000001016940011560000000000AAAAAA   message after all nibbles xor 0xA
# CCCCIIIIDDD??FGGGWWWRRRRRR??TTTBHHbbbbbbVVVttttttt
# C = LFSR-16 digest, generator 0x8810 key 0xba95 with a final xor 0x6df1, which likely means we got that wrong.
# I = station ID
# D = wind direction in degree, BCD coded, DDD = 158 => 158 °
# F = flags, 4 bit
#     Bit:    0123
#             1010
#             r???
#             r:   1 bit device reset, 1 after inserting battery
#             ???: always 010
# G = wind gust in 1/10 m/s, BCD coded, GGG = 123 => 12.3 m/s.
# W = wind speed in 1/10 m/s, BCD coded, WWW = 123 => 12.3 m/s.
# R = rain counter, in 0.1 mm, BCD coded RRRRRR = 000084 => 8.4 mm
# T = temperature in 1/10 °C, BCD coded, TTT = 312 => 31.2 °C
# B = battery. 0=Ok, 6=Low
# H = humidity in percent, BCD coded, HH = 23 => 23 %
# b = brightness, BCD coded, BBBBBB = 005584 => 5.584 klx
# V = uv, BCD coded, VVV = 012 => 1.2
# ? = unknown
# t = trailer
*/
