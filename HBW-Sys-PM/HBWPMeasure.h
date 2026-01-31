/*
 * HBWPMeasure.h
 *
 * Created (www.loetmeister.de): 01.11.2025
 * 
 * Reading of I²C INA236 chip
 * using customized SBC-DVA.h library
 *
 * Provide VOLTAGE, CURRENT and POWER readings. Peering with actor allows actions when alarm is set or cleared.
 * When enabled, short key event is send on alarm state, long key event when cleared. (short = alarm; long = alarm clear)
 */

#ifndef HBWPMeasure_h
#define HBWPMeasure_h

// #include <inttypes.h>
#include <HBWired.h>
#include "src/SBC-DVA/SBC_DVA.h"  // include customized lib (see SBC-DVA subfolder for more details)

// #define DEBUG_OUTPUT


// config, address step 16
struct hbw_config_power_measure {
  uint8_t enabled:1;   // default yes
  uint8_t n_key_event_alert:1;   // send key event when alarm or error occours. Disabled by default
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
static const byte PM_SAMPLE_COUNT = 4; // number of samples for moving everage

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
    HBWPMeasure(hbw_config_power_measure* _config, SBCDVA* _sensor, TwoWire* _wire);
    // HBWPMeasure(hbw_config_power_measure* _config, SBCDVA* _sensor, TwoWire* _wire, HBWKeyVirtual* _virt_alarm_channels[3]);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual uint8_t get(uint8_t* data);
	// virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data); //TODO set() alarm level?
    virtual void afterReadConfig();
    

  private:
    hbw_config_power_measure* config;
    SBCDVA* sensor;  // pointer to call sensor functions
    // use factor 100 to keep two decimal places without float type
    uint16_t centiAvgValue[3] = {0, 0, 0};  // store 'VOLTAGE,   CURRENT,     POWER' - see "enum val_id"
    int32_t centiSumValue[3] = {0, 0, 0};
    uint16_t centiSamples[3][PM_SAMPLE_COUNT] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};  // store values for average calculation
    uint8_t sampleCount, avgIndex;
    bool onInit;
    bool sendKeyEvent;
    uint8_t keyPressNum;
    unsigned long lastSentTime, lastSampleMillis;
    

    union state_flags {
      struct s_state_flags {
        uint8_t notUsed :4; // lowest 4 bit are not used (see XML ALARM_FLAGS frame definition)
        uint8_t alert_v_over  :1; // bus voltage over limit
        uint8_t alert_power   :1; // power limit exeeded
        uint8_t alert_v_under :1; // bus voltage under limit
        uint8_t error   :1; // sensor or some other error
      } state;
      uint8_t byte:8;
    };
    state_flags status, lastStatus;

    enum val_id {
      VOLTAGE = 0,
      CURRENT,
      POWER
    };

    // get the different readings and return as "centi-" values (factor 100)
    uint16_t read_sensor_values(byte _reading)
    {
      float readingVal = 0;
      switch (_reading) {
        case val_id::CURRENT:
          readingVal = sensor->read_current();
          break;
        case val_id::POWER:
          readingVal = sensor->read_power();
          break;
        case val_id::VOLTAGE:
          readingVal = sensor->read_bus_voltage();
          break;
      }
      return ((readingVal > 0) ? (uint16_t)(readingVal *100) : 0);
    };
    
    inline void init_sensor(TwoWire* _wire)
    {
      _wire->begin();
      _wire->setClock(100000);
      sensor->init_ina236(7, 4, 4, 2, 0);
      sensor->calibrate_ina236();
      sensor->mask_enable(3);  // v bus under limit (not used, but this should keep the alert LED off)
      sensor->write_alert_limit(); // 0.66V?
    };

};

#endif
