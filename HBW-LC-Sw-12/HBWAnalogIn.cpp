
#include "HBWAnalogIn.h"


// Analog input (ADC)
HBWAnalogIn::HBWAnalogIn(uint8_t _pin, hbw_config_analog_in* _config) {
	pin = _pin;
	config = _config;
	nextReadDelay = 8800;  // TODO: set meaningfull value
	lastReadTime = 0;
	currentValue = 0;
};


// channel specific settings or defaults
void HBWAnalogIn::afterReadConfig() {
    pinMode(pin,INPUT);
    if (config->sample_interval == 0xFF) config->sample_interval = 20;		// TODO: set meaningfull value
}


uint8_t HBWAnalogIn::get(uint8_t* data) {
   // MSB first
   *data++ = ( currentValue >> 8 ) & 0xFF;
   *data = currentValue & 0xFF;
   return 2;
};


void HBWAnalogIn::loop(HBWDevice* device, uint8_t channel) {
  
  if (!config->enabled) {   // skip disabled channels
    currentValue = 0;
  	return;
  }

  
  if(millis() - lastReadTime < nextReadDelay) return;
  lastReadTime = millis();
  
  currentValue = analogRead(pin);
  //TODO: add average calculation over config->samples
  //  nextReadDelay = (config->sample_interval / config->samples) * 1000; // ???
  
  nextReadDelay = config->sample_interval * 1000;
	
}

