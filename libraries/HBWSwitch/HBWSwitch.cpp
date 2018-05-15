
#include "HBWSwitch.h"

#define OFF_STATE 0
#define ON_STATE 1
#define UNKNOWN_STATE 0xFE

// Switches
HBWSwitch::HBWSwitch(uint8_t _pin, hbw_config_switch* _config) {
    pin = _pin;
    config = _config;
    nextFeedbackDelay = 0;
    lastFeedbackTime = 0;
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
		}
		else {   // on or off
			if (*data)
				digitalWrite(pin, LOW ^ config->n_inverted);	// on (if not inverted)
			else
				digitalWrite(pin, HIGH ^ config->n_inverted);	// off (if not inverted)
		}
		currentState = !(digitalRead(pin) ^ config->n_inverted);	// update logical state
	}
	// Logging
	if(!nextFeedbackDelay && config->logging) {
		lastFeedbackTime = millis();
		nextFeedbackDelay = device->getLoggingTime() * 100;
	}
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
    if(!nextFeedbackDelay) return;
    unsigned long now = millis();
    if(now - lastFeedbackTime < nextFeedbackDelay) return;
    lastFeedbackTime = now;  // at least last time of trying
    // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
	// we know that the level has only 1 byte here
	uint8_t level;
    get(&level);	
    uint8_t errcode = device->sendInfoMessage(channel, 1, &level);   
    if(errcode == 1) {  // bus busy
    // try again later, but insert a small delay
    	nextFeedbackDelay = 250;
    }else{
    	nextFeedbackDelay = 0;
    }
}

