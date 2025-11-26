/*
 * HBWPMeasure.cpp
 *
 * Created (www.loetmeister.de): 01.11.2025
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
  lastSentTime = 0;
  status.byte = 0;
  lastStatus.byte = 0;
  onInit = true;
  avgIndex = 0;
  sampleCount = 0;
  keyPressNum = 0;
  sendKeyEvent = false;

  init_sensor(_wire);
};


void HBWPMeasure::afterReadConfig()
{
  if (config->sample_rate == 0xFF) config->sample_rate = 60;  // 30 seconds
  if (config->send_min_interval == 0xFFFF) config->send_min_interval = 60;  // 60 seconds
  if (config->send_max_interval == 0xFFFF) config->send_max_interval = 600;  // 600 seconds
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
    status.byte = 0;
    sendKeyEvent = false;
    return;  // skip disabled channels
  }
  
  // allow peering with external actors. Send independently from info message, to send when bus is idle
  if (!config->n_key_event_alert && sendKeyEvent) {
    if (device->sendKeyEvent(channel, keyPressNum, !status.byte) != HBWDevice::BUS_BUSY) {
      keyPressNum++;
      sendKeyEvent = false;  // TODO: add retry counter? (give up after x failed retries?)
    }
  }

  unsigned long now = millis();

  // read sensor based on sample rate. Skip delay on startup
  if (now - lastSampleMillis >= (unsigned long)(config->sample_rate *500) || onInit)
  {
    lastSampleMillis = now;
    onInit = false;

    if (sensor->read_device_ID() == 0) {
      status.state.error = true;
      #ifdef DEBUG_OUTPUT
        hbwdebug(F("Sensor fail!\n"));
      #endif
      //TODO reset readings?
    }
    else {
      status.state.error = false;

      /* calculate the (moving) average of the last n results */
      if (sampleCount < PM_SAMPLE_COUNT) sampleCount++;  // avoid to calculate the average on array elements that have not been updated with a reading
      
      for (uint8_t readingValID = 0; readingValID <= sizeof(val_id); readingValID++)  // loop through all readings (val_id)
      {
        centiSumValue[readingValID] -= centiSamples[readingValID][avgIndex];  // subtract old value
        centiSamples[readingValID][avgIndex] = read_sensor_values(readingValID);  // update buffer with current reading
        centiSumValue[readingValID] += centiSamples[readingValID][avgIndex];  // add new value
        
        if (centiSumValue[readingValID] > 0)  // value should always be unsigned
          centiAvgValue[readingValID] = (uint16_t)(centiSumValue[readingValID] / sampleCount);  // calculate average
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
  
  /* check if anything needs to be send on the bus */
  // never send before min_interval, if enabled
  if (config->send_min_interval && now - lastSentTime <= (unsigned long)config->send_min_interval *1000)  return;

  // send, if max interval (wait time) has passed, or any new alarms / states came up
  if ((config->send_max_interval && now - lastSentTime >= (unsigned long)config->send_max_interval *1000) ||
      // (other urgent new value?) ||
      (status.byte != 0 && status.byte != lastStatus.byte) )
  {
    // key event only for alert state changes
    if (status.byte != lastStatus.byte) {
      sendKeyEvent = true;
    }

    uint8_t data[P_MEASURE_DATA_LEN];
    get(data);
    if (device->sendInfoMessage(channel, sizeof(data), data) != HBWDevice::BUS_BUSY) {
      lastStatus.byte = status.byte;   // store last value only on success
    }
    lastSentTime = now;  // if send failed, next try will be on send_max_interval or send_min_interval in case the values are still different
  }

};

