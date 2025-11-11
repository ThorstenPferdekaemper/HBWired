/*
 * HBWPMeasure.h
 *
 * Created (www.loetmeister.de): 01.09.2025
 * 
 * Reading of I²C INA236 chip
 * using SBC-DVA.h library
 */

#ifndef HBWPMeasure_h
#define HBWPMeasure_h

// #include <inttypes.h>
#include <HBWired.h>
#include <SBC_DVA.h>

#define DEBUG_OUTPUT

#define PM_SAMPLE_COUNT 4  // number of samples for moving everage

// config, address step 16
struct hbw_config_power_measure {
  uint8_t enabled:1;   // default yes
  uint8_t n_key_event_alert:1;   // send key event when alarm or error occours (short = alarm; long = alarm clear)
  uint8_t fillup:6;
  uint8_t dummy8;
  // uint8_t samples:4;    // 1...16 (0...15 +1) sample count
  // uint8_t fillup2:4;    // 
  uint8_t fillup2;
  uint8_t sample_rate;    // 0.5...127 s ?? (500 ms stepping? 4 samples *0.5 = 2 sec. min rate)
  uint16_t send_min_interval;       // Minimum-Sendeintervall
  uint16_t send_max_interval;       // Maximum-Sendeintervall
  uint16_t alarm_v_limit_upper;      // max voltage alarm level (factor 100)
  uint16_t alarm_v_limit_lower;      // min voltage alarm level (factor 100)
  uint16_t alarm_p_limit;            // max power alarm level (factor 100)
  uint16_t dummy16;
};

static const byte P_MEASURE_DATA_LEN = 7; // size of get() return data array

// config, address step 1
// struct hbw_config_power_measure_alarm {
//   uint8_t n_notify:1;   // default no
//   uint8_t fillup:7;
// };

// Class HBWPMeasure virtual alarm channel TODO: how to link??
// class HBWPMeasure : public HBWChannel {
//   public:
//     HBWPMeasure(hbw_config_power_measure_alarm* _config);
//     // virtual void loop(HBWDevice*, uint8_t channel);
//     // virtual uint8_t get(uint8_t* data);
// 	// virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data); //TODO set() alarm level?
//     // virtual void afterReadConfig();
    
//   protected:
//     bool alarmState;

//   private:
//     hbw_config_power_measure_alarm* config;
//     // uint8_t resetPin; // output pin
//     // unsigned long loopPreviousMillis;
//   private:
//     static byte numTotal;    // count total number of virtual channels

//   public:
//     static inline void addNumVChannel()
//     {
//       numTotal++;
//     }
//     static inline uint8_t getNumVChannel()
//     {
//       return numTotal;
//     }
// };

// Class HBWPMeasure master channel (actual sensor chan)
class HBWPMeasure : public HBWChannel {
  public:
    HBWPMeasure(hbw_config_power_measure* _config, SBCDVA* _sensor, TwoWire* _wire); // _alarm_channels = 0
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
	// virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data); //TODO set() alarm level?
    virtual void afterReadConfig();
    

  private:
    hbw_config_power_measure* config;
    SBCDVA* sensor;  // pointer to call sensor funtions
    // int16_t deziCurrent;  // factor 10
    // int16_t deziVoltage;  // factor 10
    // int16_t deziPower;  // factor 10
    // use factor 100 to keep two decimal places without float type
    uint16_t centiAvgValue[3] = {0, 0, 0};  // store 'VOLTAGE,   CURRENT,     POWER' - see "enum val_id"
    int32_t centiSumValue[3] = {0, 0, 0};
    uint16_t centiSamples[3][PM_SAMPLE_COUNT] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};  // store values for average calculation
    uint8_t sampleCount, avgIndex;
    uint8_t keyPressNum;
    bool onInit;
    // unsigned long loopPreviousMillis;
    unsigned long lastSentTime, lastSampleMillis;

    // static const uint32_t FOO_BAR = 150;  // ms

    union state_flags {
      struct s_state_flags {
        uint8_t notUsed :4; // lowest 4 bit are not used, based on XML state_flag definition
        uint8_t alert_v_over  :1; // bus voltage over limit
        uint8_t alert_power   :1; // power limit exeeded
        uint8_t alert_v_under :1; // bus voltage under limit
        uint8_t error   :1; // sensor or some other error
      } state;
      uint8_t byte:8;
    };
    state_flags status, lastStatus;

    enum val_id {
      VOLTAGE,
      CURRENT,
      POWER
    };

    // get the different readings and return as "centi-" values (factor 100)
    uint16_t read_sensor_values(byte _reading)
    {
      float readingVal = 0;
      switch (_reading) {
        case val_id::CURRENT:
          // return (int16_t)(sensor->read_current() *10);
          readingVal = sensor->read_current();
          break;
        case val_id::POWER:
          // return (int16_t)(sensor->read_power() *10);
          readingVal = sensor->read_power();
          break;
        case val_id::VOLTAGE:
          // return (int16_t)(sensor->read_bus_voltage() *10);
          readingVal = sensor->read_bus_voltage();
          break;
      }
      // return 0;
      return readingVal > 0 ? (uint16_t)(readingVal *100) : 0;
    };
};

#endif
