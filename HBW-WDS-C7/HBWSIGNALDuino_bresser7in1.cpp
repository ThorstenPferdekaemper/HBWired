// Homematic Wired Hombrew (HBW), with SIGNALDuino
// loetmeister.de - 2024.07.14

#include "HBWSIGNALDuino_bresser7in1.h"
#include <HBW_eeprom.h>
#include "bit_util.h"


#define SENSOR_TYPE_WEATHER   1

// constructor
HBWSIGNALDuino_bresser7in1::HBWSIGNALDuino_bresser7in1(uint8_t* _msg_buffer_ptr, uint8_t* _hbw_link,
                                                      hbw_config_signalduino_wds_7in1* _config, uint16_t _eeprom_address_start) {
   msg_buffer_ptr = _msg_buffer_ptr;
   hbw_link = _hbw_link;
   config = _config;
   eeprom_address_start = _eeprom_address_start;  // TODO: external / static const? ADDRESSSTART_WDS_CONF
   currentTemp = DEFAULT_TEMP;
   lastSentTemp = 0;
   msgCounter = hbw_link[hbw_link_pos::MSG_COUNTER];  // skip first message
   stormy = false;
   lastStormy = false;
   stormyTriggerCounter = config->storm_readings_trigger;
   lastSentTime = 20000;  // wait 20 seconds after statup
   msgTimeout = true;  // true until we get a message
  //  avgSampleIdx = 0;
  //  avgSamples = 0;
};


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWSIGNALDuino_bresser7in1::afterReadConfig() {
  if (config->send_max_interval == 0xFFFF) config->send_max_interval = 300; // 5 minutes
  if (config->send_min_interval == 0xFFFF) config->send_min_interval = 30;
  if (config->send_delta_temp == 0xFF) config->send_delta_temp = 10;  // 1.0 °C
  if (config->storm_threshold_level > 30) config->storm_threshold_level = 16;  // 80 km/h
  if (config->timeout_rx > 30) config->timeout_rx = 6;  // 96 seconds

 #if defined (HBW_CHANNEL_DEBUG)
  hbwdebug(F("wds_7-1 conf - id: "));hbwdebug(config->id);hbwdebug(F("\n"));
  hbwdebug(F("wds_7-1 conf - max_interval: "));hbwdebug(config->send_max_interval);hbwdebug(F("\n"));
  hbwdebug(F("wds_7-1 conf - min_interval: "));hbwdebug(config->send_min_interval);hbwdebug(F("\n"));
  hbwdebug(F("wds_7-1 conf - delta_temp: "));hbwdebug(config->send_delta_temp);hbwdebug(F("\n"));
  hbwdebug(F("wds_7-1 conf - storm_lvl: "));hbwdebug(config->storm_threshold_level*5);hbwdebug(F("\n"));
  hbwdebug(F("wds_7-1 conf - storm_trig: "));hbwdebug(config->storm_readings_trigger);hbwdebug(F("\n"));
 #endif
};


/* required public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWSIGNALDuino_bresser7in1::get(uint8_t* data) {
  // map 360 degree wind direction to 0...16 lookup values
  uint8_t windDirState = round((float)(windDir / 22.5));

  u_state_and_wdir stateAndWdir;
  stateAndWdir.field.windDir = (windDirState < 16) ? windDirState : 0;  // send only 0..15, as 0 and 16 are same direction
  stateAndWdir.field.battOk = batteryOk;
  stateAndWdir.field.timeout = msgTimeout;
  stateAndWdir.field.storm = stormy;

  // for multi byte values, MSB first
  // write value and increment pointer for next value
  *data++ = (currentTemp >> 8);
  *data++ = currentTemp & 0xFF;
  *data++ = humidityPct;
  *data++ = (rainMmRaw >> 16);
  *data++ = (rainMmRaw >> 8);
  *data++ = rainMmRaw & 0xFF;
  *data++ = (windMaxMsRaw >> 8);
  *data++ = windMaxMsRaw & 0xFF;
  *data++ = (windAvgMsRaw >> 8);
  *data++ = windAvgMsRaw & 0xFF;
  *data++ = stateAndWdir.byte;
  *data++ = (lightLuxDeci >> 8);
  *data++ = lightLuxDeci & 0xFF;
  *data++ = uvIndex;
  *data = rssiDb;

  return WDS_7IN1_DATA_LEN;
};


/* main channel loop, called by device loop */
void HBWSIGNALDuino_bresser7in1::loop(HBWDevice* device, uint8_t channel) {
  // check for new message. Copy message directly, else buffer might be overwriten by other sender
  if (hbw_link[hbw_link_pos::MSG_COUNTER] != msgCounter) {
    memset(message_buffer, 0, sizeof(message_buffer));  // make sure it's clean
    memcpy(message_buffer, msg_buffer_ptr, sizeof(message_buffer));
    rssi_raw = hbw_link[hbw_link_pos::RSSI];
    msgCounter = hbw_link[hbw_link_pos::MSG_COUNTER];  // remember counter to not process same msg again

    if (parseMsg() == SUCCESS) {  // new message for this sensor channel!
      msgTimeout = false;
      lastMsgTime = millis();
      // manage index for average calculation
      // if (config->average_samples) {
      //   if (avgSamples < WDS_7IN1_AVG_SAMPLES)  avgSamples++;  // avoid to calculate the average on array elements that have not been updated with a reading
      //   if (avgSampleIdx < WDS_7IN1_AVG_SAMPLES)  avgSampleIdx++;
      //   else avgSampleIdx = 0;
      // }
      // else {
      //   avgSampleIdx = 0;  // only use single/last value
      //   avgSamples = 1;
      // }

      // check new storm status, if enabled
      // TODO: check if windMax or Avg should be used
      if (config->storm_threshold_level) {
        if ((float)windMaxMsRaw *0.36 >= config->storm_threshold_level *5) {
          if (stormyTriggerCounter > 0)  stormyTriggerCounter--;
          else stormy = true;
        }
        else {
          if (stormyTriggerCounter < config->storm_readings_trigger)  stormyTriggerCounter++;
          else stormyTriggerCounter = config->storm_readings_trigger;  // in case config changed
          if (stormyTriggerCounter == config->storm_readings_trigger) {
          // not stormy (anymore) reset state
          stormy = lastStormy = false;
          }
        }
        #if defined (HBW_CHANNEL_DEBUG)
        hbwdebug(F("stormy counter: "));hbwdebug(stormyTriggerCounter);hbwdebug(F("\n"));
        #endif
      }
    }
  }

  // if (millis() - lastCheck >= 500 && printDebug) {  // delay output (check Serial is free?)
    // lastCheck = millis();
    // printDebug = false;
  // }
  
  unsigned long now = millis();

  // check message timeout. Skip after init (currentTemp == DEFAULT_TEMP) or if disabled in config
  if (currentTemp != DEFAULT_TEMP && config->timeout_rx && now - lastMsgTime > (unsigned long)config->timeout_rx *16000) {
    msgTimeout = true;
    currentTemp = ERROR_TEMP;
  }

  // check for new values. Only send on the bus, if values changed as set by delta or max send invervall is passed
  // never send before min_interval, if enabled
  if (config->send_min_interval && now - lastSentTime <= (unsigned long)config->send_min_interval *1000)  return;
  // int32_t currentTempAvg = 0;
  // unint16_t humidityPctAvg = 0;
  // for (i=0; i < avgSamples; i++) {
  //   currentTempAvg += currentTemp[i];
  //   humidityPctAvg += humidityPct[i];
  // }
  // currentTempAvg = currentTempAvg / avgSamples;
  // humidityPctAvg = humidityPctAvg / avgSamples;

  if ((config->send_max_interval && now - lastSentTime >= (unsigned long)config->send_max_interval * 1000) ||
      (config->send_delta_temp && abs(currentTemp - lastSentTemp) >= (unsigned int)config->send_delta_temp * 10) ||
      // (other urgent new value?) ||
      (stormy && stormy != lastStormy) )
  {
    uint8_t data[WDS_7IN1_DATA_LEN];
    get(data);
    if (device->sendInfoMessage(channel, sizeof(data), data) != HBWDevice::BUS_BUSY) {
     #ifdef Support_HBWLink_InfoEvent
      // only send temperature value for peerings (first 2 bytes of data[14])
      device->sendInfoEvent(channel, 2, data, !NEED_IDLE_BUS);  // send peerings. Info message has just been send, so we send immediately
     #endif
      lastSentTemp = currentTemp;   // store last value only on success
      lastStormy = stormy;
    }
    lastSentTime = now;  // if send failed, next try will be on send_max_interval or send_min_interval in case the values are still different
  }
};

/* parse raw message, validate and extract actual values */
HBWSIGNALDuino_bresser7in1::msg_parser_ret_code HBWSIGNALDuino_bresser7in1::parseMsg() {

  /* src: https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_7in1.c */

  int s_type  = message_buffer[6] >> 4;

  // data whitening
  for (unsigned char i = 0; i < sizeof (message_buffer); ++i) {
      message_buffer[i] ^= 0xAA;
  }
 #if defined (HBW_CHANNEL_DEBUG)
  delay(500);  // TODO fixme: add own debug output in loop - avoid conflict on same serial output!
  hbwdebug(F("type: "));hbwdebughex(s_type);hbwdebug(F("\n"));
 #endif

  // LFSR-16 digest, generator 0x8810 key 0xba95 final xor 0x6df1
  int chk    = (message_buffer[0] << 8) | message_buffer[1];
  int digest = lfsr_digest16(&message_buffer[2], 23, 0x8810, 0xba95);
  if ((chk ^ digest) != 0x6df1) {
     #if defined (HBW_CHANNEL_DEBUG)
      hbwdebug(F("Digest check failed!\n"));
     #endif
      return DECODE_FAIL_MIC;
  }

  int id          = (message_buffer[2] << 8) | (message_buffer[3]);
  int flags       = (message_buffer[15] & 0x0f);
  bool battery_low = (flags & 0x06) == 0x06;

  if (s_type == SENSOR_TYPE_WEATHER) {
    if (config->id == 0x0 || config->id == 0xFFFF) {
      // if no ID was saved, use the one we got now
      config->id = id;
      EepromPtr->update(eeprom_address_start, (uint8_t)id);
      EepromPtr->update(eeprom_address_start +1, (uint8_t)(id>>8));  // must match address of config struct (hbw_config_signalduino_wds_7in1->id)
      }
    if (id != config->id)
      return ID_MISSMATCH;
    
    int wdir     = (message_buffer[4] >> 4) * 100 + (message_buffer[4] & 0x0f) * 10 + (message_buffer[5] >> 4);
    int wgst_raw = (message_buffer[7] >> 4) * 100 + (message_buffer[7] & 0x0f) * 10 + (message_buffer[8] >> 4);
    float wgst = wgst_raw * 0.1f; // wind_max_m_s "Wind Gust", "%.1f m/s"
    int wavg_raw = (message_buffer[8] & 0x0f) * 100 + (message_buffer[9] >> 4) * 10 + (message_buffer[9] & 0x0f);
    float wavg = wavg_raw * 0.1f; // wind_avg_m_s "Wind Speed", "%.1f m/s"
    int rain_raw = (message_buffer[10] >> 4) * 100000 + (message_buffer[10] & 0x0f) * 10000 + (message_buffer[11] >> 4) * 1000
            + (message_buffer[11] & 0x0f) * 100 + (message_buffer[12] >> 4) * 10 + (message_buffer[12] & 0x0f) * 1; // 6 digits
    float rain_mm = rain_raw * 0.1f;
    int temp_raw = (message_buffer[14] >> 4) * 100 + (message_buffer[14] & 0x0f) * 10 + (message_buffer[15] >> 4);
    int temp_mc = temp_raw;
    if (temp_raw > 600)
        temp_mc = (temp_raw - 1000);
    float temp_c = temp_mc * 0.1f;
    int humidity = (message_buffer[16] >> 4) * 10 + (message_buffer[16] & 0x0f);
    int lght_raw = (message_buffer[17] >> 4) * 100000 + (message_buffer[17] & 0x0f) * 10000 + (message_buffer[18] >> 4) * 1000
            + (message_buffer[18] & 0x0f) * 100 + (message_buffer[19] >> 4) * 10 + (message_buffer[19] & 0x0f);
    int uv_raw =   (message_buffer[20] >> 4) * 100 + (message_buffer[20] & 0x0f) * 10 + (message_buffer[21] >> 4);

    // store values
    windDir = wdir;
    windMaxMsRaw = wgst_raw;
    windAvgMsRaw = wavg_raw;
    rainMmRaw = rain_raw;  // max size 166666.5? 7 digits, 21 bits?
    lightLuxDeci = lght_raw /10;  // ingnore lowest digit and transfer as 2 byte value. Allowing max value 655,350
    currentTemp = temp_mc *10; // temperature from milli to centi celsius (HM uses factor 100. e.g. 2000 == 20.00 °C)
    humidityPct = humidity;
    uvIndex = uv_raw;
    batteryOk = !battery_low;
    int rssi_tmp = HBWSIGNALDuino_calcRSSI(rssi_raw);
    // check limits, as we only tranfer as 1 Byte value
    rssiDb = rssi_tmp <= 127 ? rssi_tmp : 127;
    rssiDb = rssi_tmp >= -127 ? rssi_tmp : -127;

 #if defined (HBW_CHANNEL_DEBUG)
  float uv_index = uv_raw * 0.1f;
  hbwdebug(F("id: "));hbwdebug(id);hbwdebug(F("\n"));
  hbwdebug(F("batt ok: "));hbwdebug(!battery_low);hbwdebug(F("\n"));
  hbwdebug(F("wind_dir: "));hbwdebug(wdir);
   hbwdebug(F(" wind_dir_state: "));hbwdebug((float)windDir / 22.5);
   hbwdebug(F(" rounded: "));hbwdebug(round((float)windDir / 22.5));hbwdebug(F("\n"));
  hbwdebug(F("wind_max_m_s: "));hbwdebug(wgst);hbwdebug(F("\n"));
  hbwdebug(F("wind_avg_m_s: "));hbwdebug(wavg);hbwdebug(F("\n"));
  hbwdebug(F("rain_mm: "));hbwdebug(rain_mm);hbwdebug(F("\n"));
  hbwdebug(F("temp_c: "));hbwdebug(temp_c);hbwdebug(F("\n"));
  hbwdebug(F("humidity: "));hbwdebug(humidity);hbwdebug(F("\n"));
  hbwdebug(F("light_lux: "));hbwdebug(lght_raw);hbwdebug(F("\n"));
  hbwdebug(F("uv_index: "));hbwdebug(uv_index);hbwdebug(F("\n"));
  hbwdebug(F("RSSI db: "));hbwdebug(rssiDb);hbwdebug(F("\n"));
  hbwdebug(F("stormy: "));hbwdebug(stormy);hbwdebug(F("\n"));
 #endif
    return SUCCESS;
  }
 #if defined (HBW_CHANNEL_DEBUG)
  hbwdebug(F("ingnored"));hbwdebug(F("\n"));
 #endif
  return MSG_IGNORED;
}

