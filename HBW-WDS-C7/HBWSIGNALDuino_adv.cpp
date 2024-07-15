// Homematic Wired Hombrew (HBW), with SIGNALDuino
// loetmeister.de - 2024.07.14

#include "HBWSIGNALDuino_adv.h"

// def
HBWSIGNALDuino_adv::HBWSIGNALDuino_adv(uint8_t _pin_receive, uint8_t _pin_send, uint8_t _pin_led, hbw_config_signalduino_adv* _config) {
   pin_receive = _pin_receive;
   pin_led = _pin_led;
   config = _config;
  //  clearFeedback();
  //  currentState = UNKNOWN_STATE;
};


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWSIGNALDuino_adv::afterReadConfig() {
  // hbwdebug(F("SIGNALDuino_adv conf - ot: "));hbwdebug(config->offTime);hbwdebug(F("\n"));
	// if (currentState == UNKNOWN_STATE) {
	// // All off on init, but consider inverted setting
		// digitalWrite(pin, config->n_inverted ? LOW : HIGH);		// 0=inverted, 1=not inverted (device reset will set to 1!)
		// pinMode(pin,OUTPUT);
		// currentState = OFF_STATE;
	// }
	// else {
	// // Do not reset outputs on config change (EEPROM re-reads), but update its state
		// digitalWrite(pin, !currentState ^ config->n_inverted);
	// }
}


void HBWSIGNALDuino_adv::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
	// if (config->output_unlocked) {	//0=LOCKED, 1=UNLOCKED
		// if(*data > 200) {   // toggle
		// 	digitalWrite(pin, digitalRead(pin) ? LOW : HIGH);  // TODO: force offTime...
		// 	currentState = (currentState ? OFF_STATE : ON_STATE);
		// }
		// else {   // on or off
		// 	if (*data) {
		// 		if (millis() - lastOnOffTime >= 4000) {  // TODO: use config->offTime 
		// 		digitalWrite(pin, LOW ^ config->n_inverted);	// on (if not inverted)
		// 		currentState = ON_STATE;
		// 		lastOnOffTime = millis();
		// 		}
		// 	}
		// 	else {
		// 		digitalWrite(pin, HIGH ^ config->n_inverted);	// off (if not inverted)
		// 		currentState = OFF_STATE;
		// 	}
		// }
	// }
	// Logging
	// set trigger to send info/notify message in loop()
    // setFeedback(device, config->logging);
};


uint8_t HBWSIGNALDuino_adv::get(uint8_t* data) {
	// if (currentState)
		(*data) = 200;
	// else
	// 	(*data) = 0;
	return 1;
};


void HBWSIGNALDuino_adv::loop(HBWDevice* device, uint8_t channel) {
  
  // if (millis() - lastOnOffTime >= 3000 && currentState == ON_STATE) {  // TODO: use config->onTime
  //   lastOnOffTime = millis();
  //   uint8_t value = 0;
  //   set(device, 1, &value); // set off
  // }
	// feedback trigger set?
    // checkFeedback(device, channel);
};
