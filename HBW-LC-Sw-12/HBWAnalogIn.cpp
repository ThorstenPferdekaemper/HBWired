
#include "HBWAnalogIn.h"


// Analog input (ADC)
HBWAnalogIn::HBWAnalogIn(uint8_t _pin, hbw_config_analog_in* _config) {
  pin = _pin;
  config = _config;
  nextReadDelay = 800;  // TODO: set meaningfull value
  lastReadTime = 0;
  currentValue = 0;
  ADCresult_sum = 0;
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
  
  if (config->n_enabled) {   // skip disabled channels (1=disabled)
    currentValue = 0;
    return;
  }
  
  if(millis() - lastReadTime < nextReadDelay) return;
  lastReadTime = millis();
  
  //currentValue = analogRead(pin);
  //TODO: add average calculation over config->samples
  //  nextReadDelay = (config->sample_interval / config->samples) * 1000; // ???
  
  uint16_t ADCreading = analogRead(pin);
  /* calculate the ADC average of the last n results */
  ADCresult_sum += (ADCreading - ADCresult_buf[ADCindex]);  // add new and substract old value
  ADCresult_buf[ADCindex] = ADCreading;  // update buffer with current reading
  currentValue = ADCresult_sum / config->samples;  // calculate average

  ++ADCindex;
  if (ADCindex > config->samples) {
    ADCindex = 0; // reset when last array element was processed
  }
  
  nextReadDelay = config->sample_interval * 1000;

}

