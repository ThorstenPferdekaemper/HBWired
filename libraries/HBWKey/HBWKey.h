
#ifndef HBWKey_h
#define HBWKey_h
#include <inttypes.h>
#include "Arduino.h"
#include "HBWired.h"


//   +0.0 00.1 INPUT_TYPE (switch/pushbutton)
//   +0.1 00.1 INPUT_LOCKED
//   +1.0 01.0 LONG_PRESS_TIME
// TODO: input_type und input_locked implementieren
struct hbw_config_key {
	uint8_t input_type:1;
	uint8_t input_locked:1;
    uint8_t :6;
	uint8_t long_press_time;              // 0x0000
};


// Class HBWKey
class HBWKey : public HBWChannel {
	public:
		HBWKey(uint8_t _pin, hbw_config_key* _config);
		virtual void loop(HBWDevice*, uint8_t channel);
	private:
  	    uint8_t pin;   // Pin
		uint32_t keyPressedMillis;  // Zeit, zu der die Taste gedrueckt wurde (fuer's Entprellen)
		uint32_t lastSentLong;      // Zeit, zu der das letzte Mal longPress gesendet wurde
		uint8_t keyPressNum;
		hbw_config_key* config;
};

#endif 