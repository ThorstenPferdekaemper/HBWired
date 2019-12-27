/*
 * HBWOneWireTempSensors.cpp
 *
 *  Created on: 02.04.2019
 *      Author: loetmeister.de Vorlage: hglaser, thorsten@pferdekaemper.com, Hoschiq
 */

#include "HBWOneWireTempSensors.h"
#include <EEPROM.h>


HBWOneWireTemp::HBWOneWireTemp(OneWire* _ow, hbw_config_onewire_temp* _config, uint32_t* _owLastReadTime, uint8_t* _owCurrentChannel) {
  ow = _ow;
  config = _config;
  owLastReadTime = _owLastReadTime;
  owCurrentChannel = _owCurrentChannel;
  currentTemp = DEFAULT_TEMP;
  lastSentTemp = DEFAULT_TEMP;
  lastSentTime = 0;
  errorCount = OW_DEVICE_ERROR_COUNT;
  errorWasSend = true;
}


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWOneWireTemp::afterReadConfig() {
      if (config->offset == 0xFF) config->offset = 127;
      if (config->send_delta_temp == 0xFF) config->send_delta_temp = 5;
      if (config->send_max_interval == 0xFFFF) config->send_max_interval = 150;
      if (config->send_min_interval == 0xFFFF) config->send_min_interval = 10;
      
      action = ACTION_START_CONVERSION; // start with new conversion, maybe the previous sensor was removed...
      
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("conf_OW addr: "));  m_hbwdebug_ow_address(&config->address[0]);  hbwdebug(F("\n"));
  #endif
};


/*
 * search sensors and read its address
 * Only search for new sensors if there is a free channel left
 */
void HBWOneWireTemp::sensorSearch(OneWire* ow, hbw_config_onewire_temp** _config, uint8_t channels, uint8_t address_start) {
  
  if (ow == NULL)  return;
  
  #ifdef DEBUG_OUTPUT
  hbwdebug(F("OW sensorSearch\n"));
  #endif

  uint8_t addr[OW_DEVICE_ADDRESS_SIZE];
  uint8_t channel;

  // search for addresses on the bus
  ow->reset_search();
  
  while (1) {
    // get a free slot
    for (channel = 0; channel < channels; channel++) {
  #ifdef EXTRA_DEBUG_OUTPUT
  hbwdebug(F("channel: "));  hbwdebug(channel);
  hbwdebug(F(" stored address: "));  m_hbwdebug_ow_address(&_config[channel]->address[0]);  hbwdebug(F("\n"));
  #endif
      if (deviceInvalidOrEmptyID(_config[channel]->address[0])) break;   // free slot found
    }
    if (channel == channels) break;   // no free slot found
  
    // now 'channel' points to a free slot
    if (!ow->search(addr))  break;     // no further sensor found
    
  #ifdef DEBUG_OUTPUT
	hbwdebug(F("1-Wire device found: "));  m_hbwdebug_ow_address(&addr[0]);
  #endif

    if (ow->crc8(addr, 7) != addr[7]) {
  #ifdef DEBUG_OUTPUT
  hbwdebug(F(" CRC invalid-ignoring device!\n"));
  #endif
      continue;
    }
    
  // now we found a valid device, check if this is already known
    uint8_t oldChan;
    for (oldChan = 0; oldChan < channels; oldChan++) {
      if (memcmp(addr, _config[oldChan]->address, OW_DEVICE_ADDRESS_SIZE) == 0)  break;   // found
    }
    if (oldChan < channels) {
  #ifdef DEBUG_OUTPUT
  hbwdebug(F(" - already known\n"));
  #endif
      continue;
    }
  // we have a new device!
    memcpy(_config[channel]->address, addr, OW_DEVICE_ADDRESS_SIZE);
    
  
/**
 * write sensor addresses from memory into EEPROM - for new devices only
 */
    uint8_t startaddress = address_start + (sizeof(*_config[0]) - OW_DEVICE_ADDRESS_SIZE) + (sizeof(*_config[0]) * channel);
    
  #ifdef DEBUG_OUTPUT
  hbwdebug(F(" save to EEPROM, @startaddress: "));  hbwdebughex(startaddress);  hbwdebug(F("\n"));
  #endif
    byte* ptr = (byte*) (_config[channel]->address);
    for (uint8_t addr = startaddress ; addr < (startaddress + OW_DEVICE_ADDRESS_SIZE); addr++) {
      // EEPROM read, write if different
      if (*ptr != EEPROM.read(addr))  EEPROM.write(addr, *ptr);
      ptr++;
    }
    
  } // while (1)
};


/**
 * send "start conversion" to device
 */
void HBWOneWireTemp::oneWireStartConversion() {
  
  if (deviceInvalidOrEmptyID(config->address[0]))
    return;  // ignore channels without valid sensor
  ow->reset();
  ow->select(config->address);
  ow->write(0x44, 1);        // start conversion, with parasite power on at the end
};


/**
 * read one wire temperature from sensor, returns temperature in milli °C
 * return ERROR_TEMP for consecutive CRC errors or disconnected devices
 * return DEFAULT_TEMP for emtpy channels (without valid address)
 */
int16_t HBWOneWireTemp::oneWireReadTemp() {
  
  if (deviceInvalidOrEmptyID(config->address[0]))
	  return DEFAULT_TEMP;  // ignore channels without valid sensor

	uint8_t data[12];
	isAllZeros = true;

	ow->reset();
	ow->select(config->address);
	ow->write(0xBE);  // Read Scratchpad

	for (uint8_t i = 0; i < 9; i++) {      // we need 9 bytes
		data[i] = ow->read();
		if (data[i] != 0) isAllZeros = false;
	}
	// error for failed CRC check and if all bytes of scratchPad are zero (disconnected device)
	if (ow->crc8(data, 8) != data[8] || isAllZeros) {
	  if (errorCount == 0) {
	    return ERROR_TEMP;  //return ERROR temp for CRC error and disconnected devices - after 3 retries
	  }
    else {
	    if (errorCount == 1)
	      errorWasSend = false;
	    errorCount--;
		  return currentTemp;
	  }
	}
	if (errorCount < OW_DEVICE_ERROR_COUNT) {
	  errorCount = OW_DEVICE_ERROR_COUNT;	// reset error counter
	  return ERROR_TEMP;	// skip first measure (could be incomplete)
	}
	
	/**
	 * Convert the data to actual temperature
	 * because the result is a 16 bit signed integer, it should
	 * be stored to an "int16_t" type, which is always 16 bits
	 * even when compiled on a 32 bit processor.
	 */
	int16_t raw = (data[1] << 8) | data[0];
	if (config->address[0] == 0x10) {	// DS18S20 or old DS1820
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {   // DS18S20 or old DS1820
			// "count remain" gives full 12 bit resolution
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	}
	else {	// DS18B20 or DS1822
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
 * and send the actual Temperature when max_interval reached, or the Temp.
 * changed more then delta_temp but not faster than min_interval.
 */
void HBWOneWireTemp::loop(HBWDevice* device, uint8_t channel) {

  uint32_t now = millis();
  static uint8_t level[2];

  if (lastSentTime == 0)
    lastSentTime = now + (channel *OW_POLL_FREQUENCY/2);  // init with different time, to not spam the bus (will vary over time anyway...)
  
  if (*owCurrentChannel == OW_CHAN_INIT)
      *owCurrentChannel = channel;
  
  // read onewire sensor every n seconds (OW_POLL_FREQUENCY/1000)
  if ((now - *owLastReadTime >= OW_POLL_FREQUENCY) && *owCurrentChannel == channel) {
    if (action == ACTION_START_CONVERSION) {
      *owLastReadTime = now;
      oneWireStartConversion();   // start next measurement
      action = ACTION_READ_TEMP;  // next action
    }
    else if (action == ACTION_READ_TEMP) {
      currentTemp = oneWireReadTemp();   // read temperature
      action = ACTION_START_CONVERSION;  // next action
      *owCurrentChannel = OW_CHAN_INIT;    // 'reset' current channel. Next call to loop will point to next channel
      
  #ifdef EXTRA_DEBUG_OUTPUT
  hbwdebug(F("channel: "));  hbwdebug(channel);
  hbwdebug(F(" read temp, c°C: "));  hbwdebug(currentTemp);  hbwdebug(F("\n"));	// display temperatue in centi celcius incl. offset
  #endif
    }
  }
  
  // check if we have a valid temp
  if ((currentTemp == DEFAULT_TEMP) || (currentTemp == ERROR_TEMP && errorWasSend))  return; // send ERROR_TEMP in error state just once

  // check if some temperatures needed to be send
  // do not send before min interval
  if (config->send_min_interval && now - lastSentTime <= (long)config->send_min_interval * 1000)  return;
  if ((config->send_max_interval && now - lastSentTime >= (long)config->send_max_interval * 1000) ||
      (config->send_delta_temp && abs( currentTemp - lastSentTemp ) >= (unsigned int)(config->send_delta_temp) * 10)) {
    // send temperature
    get(level);
    if (device->sendInfoMessage(channel, 2, level) == HBWDevice::SUCCESS) {    // level has always 2 byte here
     #ifdef Support_HBWLink_InfoEvent
      device->sendInfoEvent(channel, 2, level, !NEED_IDLE_BUS);  // send peerings. Info message has just been send, so we send immediately
     #endif

      lastSentTemp = currentTemp;   // store last value only on success
      errorWasSend = true;
    }
    lastSentTime = now;   // if send failed, next try will be on send_max_interval or send_min_interval in case the value changed (send_delta_temp)

   #ifdef Support_HBWLink_InfoEvent
   // device->sendInfoEvent(channel, 2, level);
   #endif

  #ifdef DEBUG_OUTPUT
  hbwdebug(F("channel: "));  hbwdebug(channel);
  hbwdebug(F(" sent temp, c°C: "));  hbwdebug(lastSentTemp); lastSentTemp == currentTemp ? hbwdebug(F(" SUCCESS!\n")) : hbwdebug(F(" FAILED!\n"));
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


/* validate device ID (first byte of device address) */
bool HBWOneWireTemp::deviceInvalidOrEmptyID(uint8_t deviceType) {
  return !((deviceType == DS18B20_ID) || (deviceType == DS18S20_ID) || (deviceType == DS1822_ID));
};
