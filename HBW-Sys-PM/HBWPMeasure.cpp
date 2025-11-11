/*
 * HBWPMeasure.cpp
 *
 * Created (www.loetmeister.de): 01.09.2025
 */

#include "HBWPMeasure.h"

// // Class HBWPMeasure
// HBWPMeasure::HBWPMeasure(hbw_config_power_measure_alarm* _config)
// config(_config)
// {
//   addNumVChannel();
//   alarmState = false;
// };

// Class HBWPMeasure
HBWPMeasure::HBWPMeasure(hbw_config_power_measure* _config, SBCDVA* _sensor, TwoWire* _wire)
{
  config = _config;
  sensor = _sensor;
  // deziCurrent = 0;  // TODO: init with "invalid" values?
  // deziVoltage = 0;
  // deziPower = 0;
  lastSentTime = 0; // TODO: delay notify until we have valid readings or min delay passed??
  onInit = true;
  status.byte = 0;
  lastStatus.byte = 0;
  keyPressNum = 0;
  avgIndex = 0;
  sampleCount = 0;

  _wire->begin();
  _wire->setClock(100000);
  // sensor->reset_ina236(0x40);
  sensor->init_ina236(7, 4, 4, 2, 0);
  sensor->calibrate_ina236();
  sensor->mask_enable(3);  // v bus under limit (not used, but this should keep the alert LED off)
  sensor->write_alert_limit(); // 0.66V?
};


void HBWPMeasure::afterReadConfig()
{
  if (config->sample_rate == 0xFF) config->sample_rate = 60;  // 30 seconds
  if (config->send_min_interval == 0xFFFF) config->send_min_interval = 6;  // 60 seconds
  if (config->send_max_interval == 0xFFFF) config->send_max_interval = 60;  // 600 seconds
  if (config->alarm_v_limit_upper == 0xFFFF) config->alarm_v_limit_upper = 255;  // 25.5 V
  if (config->alarm_v_limit_lower == 0xFFFF) config->alarm_v_limit_lower = 230;  // 23.0 V
  if (config->alarm_p_limit == 0xFFFF) config->alarm_p_limit = 500;  // 50 W

#ifdef DEBUG_OUTPUT
  // sensor->print_alert_limit(hbwdebugstream);
  sensor->print_manufacturer_ID(hbwdebugstream);
  sensor->print_device_ID(hbwdebugstream);
  hbwdebug(F("cfg PM, sample rate:"));
  hbwdebug(config->sample_rate);
  hbwdebug(F(" intervals min:"));
  hbwdebug(config->send_min_interval);
  hbwdebug(F(" max:"));
  hbwdebug(config->send_max_interval);
  hbwdebug(F("\n"));
#endif
};

uint8_t HBWPMeasure::get(uint8_t* data)
{
  // MSB first
  // *data++ = (deziVoltage >> 8);
  // *data++ = deziVoltage & 0xFF;
  // *data++ = (deziCurrent >> 8);
  // *data++ = deziCurrent & 0xFF;
  // *data++ = (deziPower >> 8);
  // *data++ = deziPower & 0xFF;
  *data++ = (centiAvgValue[val_id::VOLTAGE] >> 8);
  *data++ = centiAvgValue[val_id::VOLTAGE] & 0xFF;
  *data++ = (centiAvgValue[val_id::CURRENT] >> 8);
  *data++ = centiAvgValue[val_id::CURRENT] & 0xFF;
  *data++ = (centiAvgValue[val_id::POWER] >> 8);
  *data++ = centiAvgValue[val_id::POWER] & 0xFF;
  *data = status.byte;

  return P_MEASURE_DATA_LEN;  // 7 byte
};

void HBWPMeasure::loop(HBWDevice* device, uint8_t channel)
{
  if (!config->enabled) {
    //TODO reset readings?
    return;  // skip locked channels
  }
  
  unsigned long now = millis();
  // now = (now == 0) ? 1 : now;

  // read sensor based on sample rate. Skip delay on startup
  if (now - lastSampleMillis >= (unsigned long)(config->sample_rate *500) || onInit)
  {
    lastSampleMillis = now;
    onInit = false;

    if (sensor->read_device_ID() == 0) { // FIXME: == 0
      status.state.error = true;
      #ifdef DEBUG_OUTPUT
        hbwdebug(F("Sensor fail!\n"));
      #endif
      //TODO reset readings?
    }
    else {
      status.state.error = false;

      // float currentReading = sensor->read_current();
      // // deziCurrent = (int16_t)(currentReading *10);
      // float voltageReading = sensor->read_bus_voltage();
      // // deziVoltage = (int16_t)(voltageReading *10);
      // float powerReading = sensor->read_power();
      // // deziPower = (int16_t)(powerReading *10);

      /* calculate the (moving) average of the last n results */
      if (sampleCount < PM_SAMPLE_COUNT) sampleCount++;  // avoid to calculate the average on array elements that have not been updated with a reading
      
      for (uint8_t readingValID = 0; readingValID <= sizeof(val_id); readingValID++)  // loop through all readings (val_id)
      {
        centiSumValue[readingValID] -= centiSamples[readingValID][avgIndex];  // subtract old value
        centiSamples[readingValID][avgIndex] = read_sensor_values(readingValID);  // update buffer with current reading
        centiSumValue[readingValID] += centiSamples[readingValID][avgIndex];  // add new value
        
        if (centiSumValue[readingValID] > 0)
          centiAvgValue[readingValID] = (int16_t)(centiSumValue[readingValID] / sampleCount);  // calculate average
        else
          centiAvgValue[readingValID] = 0;
      }
      avgIndex++;
      avgIndex = avgIndex % PM_SAMPLE_COUNT; // reset when last array element was processed

      // check for alarm thresholds
      if (config->alarm_p_limit != 0) status.state.alert_power = (centiAvgValue[val_id::POWER] > config->alarm_p_limit) ? true : false;
      if (config->alarm_v_limit_lower != 0) status.state.alert_v_under = (centiAvgValue[val_id::VOLTAGE] < config->alarm_v_limit_lower) ? true : false;
      if (config->alarm_v_limit_upper != 0) status.state.alert_v_over = (centiAvgValue[val_id::VOLTAGE] > config->alarm_v_limit_upper) ? true : false;
      
    #ifdef DEBUG_OUTPUT
      hbwdebug(F("Current [A]: "));  hbwdebug(sensor->read_current(), 4);  hbwdebug(F(" avg: "));  hbwdebug(centiAvgValue[val_id::CURRENT]);
      hbwdebug(F("\n Power [W]: "));  hbwdebug(sensor->read_power(), 3);
      // hbwdebug(powerReading, 3);
      hbwdebug(F(" avg: "));  hbwdebug(centiAvgValue[val_id::POWER]);
      hbwdebug(F("\n Shunt [V]: "));  hbwdebug(sensor->read_shunt_voltage(), 4);
      hbwdebug(F("\n Bus [V]: "));  hbwdebug(sensor->read_bus_voltage(), 4);  hbwdebug(F(" avg: "));  hbwdebug(centiAvgValue[val_id::VOLTAGE]);
      hbwdebug(F("\n"));
      // hbwdebug(F("alarm_v_limit_lower: "));  hbwdebug(status.state.alert_v_under);hbwdebug(F("\n"));
    #endif
    }
  }
  
  // if (now - loopPreviousMillis < 1000)  return;
  // loopPreviousMillis = now;

  /* check if anything needs to be send on the bus */
  // never send before min_interval, if enabled
  if (config->send_min_interval && now - lastSentTime <= (unsigned long)config->send_min_interval *1000)  return;

  // send, if max interval (wait time) has passed, or any new alarms / states came up
  if ((config->send_max_interval && now - lastSentTime >= (unsigned long)config->send_max_interval *1000) ||
      // (config->send_delta_temp && abs(currentTemp - lastSentTemp) >= (unsigned int)config->send_delta_temp * 10) ||
      // (other urgent new value?) ||
      (status.byte != 0 && status.byte != lastStatus.byte) )
  {
    uint8_t data[P_MEASURE_DATA_LEN];
    get(data);
    if (device->sendInfoMessage(channel, sizeof(data), data) != HBWDevice::BUS_BUSY) {
    //  #ifdef Support_HBWLink_InfoEvent
    //   // send key event here? Allow peering?
    //   device->sendInfoEvent(channel, 2, data, !NEED_IDLE_BUS);  // send peerings. Info message has just been send, so we send immediately
      // allow peering with external actors
      if (!config->n_key_event_alert) {
        if (device->sendKeyEvent(channel, keyPressNum, !status.byte) != HBWDevice::BUS_BUSY) { // TODO: check if this works... cannot skip NEED_IDLE_BUS for sendKeyEvent()
          keyPressNum++;
        }
      }
    //  #endif
      // lastSentTemp = currentTemp;   // store last value only on success
      lastStatus.byte = status.byte;
    }
    // hbwdebug(F("sbyte: "));  hbwdebug(status.byte);hbwdebug(F("\n"));
    lastSentTime = now;  // if send failed, next try will be on send_max_interval or send_min_interval in case the values are still different
  }

};

