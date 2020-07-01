
#include "HBWSwitch.h"

#define OFF_STATE 0
#define ON_STATE 1
#define UNKNOWN_STATE 0xFE

// Switches
HBWSwitch::HBWSwitch(uint8_t _pin, hbw_config_switch* _config) {
    pin = _pin;
    config = _config;
    clearFeedback();
    currentState = UNKNOWN_STATE;
};


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWSwitch::afterReadConfig() {
	if (currentState == UNKNOWN_STATE) {
	// All off on init, but consider inverted setting
		digitalWrite(pin, config->n_inverted ? LOW : HIGH);		// 0=inverted, 1=not inverted (device reset will set to 1!)
		pinMode(pin,OUTPUT);
		currentState = OFF_STATE;
	}
	else {
	// Do not reset outputs on config change (EEPROM re-reads), but update its state
		digitalWrite(pin, !currentState ^ config->n_inverted);
	}
}


void HBWSwitch::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
	if (config->output_unlocked) {	//0=LOCKED, 1=UNLOCKED
		if(*data > 200) {   // toggle
			digitalWrite(pin, digitalRead(pin) ? LOW : HIGH);
			currentState = (currentState ? OFF_STATE : ON_STATE);
		}
		else {   // on or off
			if (*data) {
				digitalWrite(pin, LOW ^ config->n_inverted);	// on (if not inverted)
				currentState = ON_STATE;
			}
			else {
				digitalWrite(pin, HIGH ^ config->n_inverted);	// off (if not inverted)
				currentState = OFF_STATE;
			}
		}
	}
	// Logging
	// set trigger to send info/notify message in loop()
    setFeedback(device, config->logging);
};


uint8_t HBWSwitch::get(uint8_t* data) {
	if (currentState)
		(*data) = 200;
	else
		(*data) = 0;
	return 1;
};


void HBWSwitch::loop(HBWDevice* device, uint8_t channel) {
	// feedback trigger set?
    checkFeedback(device, channel);
}

