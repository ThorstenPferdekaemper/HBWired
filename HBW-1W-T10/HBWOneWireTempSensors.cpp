/*
 * HBWOneWireTempSensors.cpp
 *
 *  Created on: 02.04.2019
 *      Author: loetmeister.de Vorlage: hglaser, thorsten@pferdekaemper.com, Hoschiq
 */

#include "HBWOneWireTempSensors.h"
#include <EEPROM.h>

//DS18S20 Gerätecode 0x10
//DS18B20 Gerätecode 0x28
//DS1822  Gerätecode 0x22


HBWOneWireTemp::HBWOneWireTemp(OneWire* _ow, hbw_config_onewire_temp* _config, uint32_t* _owLastReadTime, uint8_t* _owCurrentChannel, uint8_t _channels) {
  ow = _ow;
  config = _config;
  channelsTotal = _channels;
  owLastReadTime = _owLastReadTime;
  owCurrentChannel = _owCurrentChannel;
  currentTemp = DEFAULT_TEMP;
  lastSentTemp = DEFAULT_TEMP;
}


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWOneWireTemp::afterReadConfig() {
      if (config->offset == 0xFF) config->offset = 127;
      if (config->send_delta_temp == 0xFF) config->send_delta_temp = 5;
      if (config->send_max_interval == 0xFFFF) config->send_max_interval = 150;
      if (config->send_min_interval == 0xFFFF) config->send_min_interval = 10;
};


/*
 * search sensors and read its address
 * Only search for new sensors if there is a free channel left
 */
void HBWOneWireTemp::sensorSearch(OneWire* ow, hbw_config_onewire_temp** _config, uint8_t channels, uint8_t address_start, uint8_t address_step) {
  
  if(ow == NULL) return;
  
  #ifdef DEBUG_OUTPUT
  hbwdebug("OW sensorSearch\n");
  #endif

  uint8_t addr[OW_ADDRESS_SIZE];
  uint8_t channel;

  // search for addresses on the bus
  ow->reset_search();
  while (1) {
    // get a free slot
    for (channel = 0; channel < channels; channel++) {
  #ifdef EXTRA_DEBUG_OUTPUT
    hbwdebug(F("channel: "));    hbwdebug(channel);
    hbwdebug(F(" stored address: "));
    for (uint8_t i = 0; i < OW_ADDRESS_SIZE; i++) {
      hbwdebughex(_config[channel]->address[i]);
    }
    hbwdebug("\n");
  #endif
      if (_config[channel]->address[0] == 0xFF) break;   // free slot found
    }
    if (channel == channels) break;   // no free slot found
  
    // now 'channel' points to a free slot
    if (!ow->search(addr))  break;     // no further sensor found
    
  #ifdef DEBUG_OUTPUT
  	hbwdebug(F("1-Wire Device found: "));
  	for (uint8_t i = 0; i < OW_ADDRESS_SIZE; i++) {
  	  hbwdebughex(addr[i]);
  	}
  #endif

    if(ow->crc8(addr, 7) != addr[7]) {
  #ifdef DEBUG_OUTPUT
      hbwdebug(F(" CRC invalid-ignoring device!\n"));
  #endif
    continue;
    }
    
  // now we found a valid device, check if this is already known
    uint8_t oldChan;
    for (oldChan = 0; oldChan < channels; oldChan++) {
      if (memcmp(addr, _config[oldChan]->address, OW_ADDRESS_SIZE) == 0)  break;   // found
    }
    if (oldChan < channels) {
  #ifdef DEBUG_OUTPUT
      hbwdebug(F(" - already known\n"));
  #endif
      continue;
    }
  // we have a new device!
    memcpy(_config[channel]->address, addr, OW_ADDRESS_SIZE);
    
  
/**
 * write Sensor addresses from Memory into EEPROM - for new devices only
 */
    uint8_t startaddress = address_start + (address_step - OW_ADDRESS_SIZE) + (address_step * channel);
    //uint8_t startaddress = ((uint8_t *) &_config[channel]->address) - ((uint8_t *) config);
        
    #ifdef DEBUG_OUTPUT
      hbwdebug("\nwrite Sensor: ");
      for (uint8_t i = 0; i < OW_ADDRESS_SIZE; i++) {
        hbwdebughex(_config[channel]->address[i]);
      }
      hbwdebug(" startaddress: "); hbwdebughex(startaddress);
      hbwdebug(" into EEprom\n");
    #endif
    byte* ptr = (byte*) (_config[channel]->address);
    for (uint8_t addr = startaddress ; addr < (startaddress + OW_ADDRESS_SIZE); addr++) {
      // EEPROM read, write if different
      if(*ptr != EEPROM.read(addr)) EEPROM.write(addr, *ptr); //  hbwdebug("EEwrite\n");//
      ptr++;
    }
    
  } // while (1)
};


/**
 * send "start conversion" to device
 */
void HBWOneWireTemp::oneWireStartConversion() {
  if(config->address[0] == 0xFF)
    return;  // ignore channels without sensor
  ow->reset();
  ow->select(config->address);
  ow->write(0x44, 1);        // start conversion, with parasite power on at the end
};

/**
 * read one wire temp from temp sensors
 */
int16_t HBWOneWireTemp::oneWireReadTemp() {

	if (config->address[0] == 0xFF)
	    return DEFAULT_TEMP;  // ignore channels without sensor

	uint8_t data[12];

	ow->reset();
	ow->select(config->address);
	ow->write(0xBE);  // Read Scratchpad

	for (uint8_t i = 0; i < 9; i++) {      // we need 9 bytes
		data[i] = ow->read();
	}
	// CRC check  TODO: if this happens only once, maybe don't do it
	if(ow->crc8(data, 8) != data[8])
	return ERROR_TEMP;

	/**
	 * Convert the data to actual temperature
	 * because the result is a 16 bit signed integer, it should
	 * be stored to an "int16_t" type, which is always 16 bits
	 * even when compiled on a 32 bit processor.
	 */
	//int16_t raw = (data[1] << 8) | data[0];
	int16_t raw = (data[1] << 8) | data[0];
	if (config->address[0] == 0x10) {	// DS18S20 or old DS1820
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {   // DS18S20 or old DS1820
			// "count remain" gives full 12 bit resolution
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	}
	else {	// DS18B20 or DS1822	/TODO: check first ROM byte properly. Don't try conversion on unsupported device
		uint8_t cfg = (data[4] & 0x60);
		// at lower res, the low bits are undefined, so let's zero them
		if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
		// default is 12 bit resolution, 750 ms conversion time
	}
	return (((float)raw / 16.0) * 100) + (config->offset -127);
}


/*
 * handle Temp Sensors in a loop. Read the OneWire Sensors every x Seconds
 * and send the actual Temperature when max_intervall reached, or the Temp.
 * changed more then delta_temp but not faster than min_interval.
 */
void HBWOneWireTemp::loop(HBWDevice* device, uint8_t channel) {

  uint32_t now = millis();

  //TODO: use config->send_max_interval == 0 to disable a channel?

  if (lastSentTime == 0)
    lastSentTime = now + (channel *OW_POLL_FREQUENCY/2);  // init with different time (will vary over time anyway...)
  
  // read onewire sensor every _ seconds
    if(now - *owLastReadTime >= OW_POLL_FREQUENCY && *owCurrentChannel == channel) {
      if(*owLastReadTime != 0) { // special for the first call  // TODO: rollover issue?
  	    currentTemp = oneWireReadTemp();   // read temperature
        
  #ifdef EXTRA_DEBUG_OUTPUT
    hbwdebug(F("channel: "));
    hbwdebug(channel);
    hbwdebug(F(" read temp, m°C: "));
    hbwdebug(currentTemp);
    hbwdebug("\n");
  #endif
  
  	 }
     *owLastReadTime = now;
     (*owCurrentChannel)++;
     if (*owCurrentChannel >= channelsTotal) *owCurrentChannel = 0;
  	 oneWireStartConversion();   // start next measurement
    }
    // check if we have a valid temp
    if ((currentTemp == DEFAULT_TEMP) || (currentTemp == ERROR_TEMP)) return;

    // check if some temperatures needed to be send
    // do not send before min interval
    if (config->send_min_interval && now - lastSentTime < (long)config->send_min_interval * 1000) return;
    if((config->send_max_interval && now - lastSentTime >= (long)config->send_max_interval * 1000) ||
    (config->send_delta_temp && abs( currentTemp - lastSentTemp ) >= (unsigned int)(config->send_delta_temp) * 10)) {
	// send temperature
	  uint8_t level;
    get(&level);
    device->sendInfoMessage(channel, 2, &level);    // level has 2 byte here

	  // TODO: Peering/Send key event???
    
    lastSentTemp = currentTemp;
    lastSentTime = now;
   
  #ifdef DEBUG_OUTPUT
    hbwdebug(F("channel: "));
    hbwdebug(channel);
    hbwdebug(F(" sent temp, m°C: "));
    hbwdebug(lastSentTemp);
    hbwdebug("\n");
  #endif
    }
};


/* standard public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWOneWireTemp::get(uint8_t* data) {
  
  // MSB first
  *data++ = (currentTemp >> 8);
  *data = currentTemp & 0xFF;
  return 2;
};
