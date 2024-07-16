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
   lastSend = 0;
   msgCounter = hbw_link[hbw_link_pos::MSG_COUNTER];  // skip first message
};


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWSIGNALDuino_bresser7in1::afterReadConfig() {
  if (config->send_max_interval == 0xFFFF) config->send_max_interval = 300;
  if (config->send_min_interval == 0xFFFF) config->send_min_interval = 30;

 #if defined (HBW_CHANNEL_DEBUG)
  hbwdebug(F("wds_7-1 conf - id: "));hbwdebug(config->id);hbwdebug(F("\n"));
 #endif
};


// void HBWSIGNALDuino_bresser7in1::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
// 	if (config->output_unlocked) {	//0=LOCKED, 1=UNLOCKED
		// foo
// };

/* required public function - returns length of data array. Data array contains current channel reading */
uint8_t HBWSIGNALDuino_bresser7in1::get(uint8_t* data) {
  // map 360 degree wind direction to 0...16 lookup values
  uint8_t windDirState = round((float)(windDir / 22.5));
  windDirState = windDirState < 17 ? windDirState : 0;

  u_state_and_wdir stateAndWdir;
  stateAndWdir.field.windDir = windDirState;
  stateAndWdir.field.battOk = batteryOk;

  // for 2 byte values, MSB first
  // write value and increment pointer for next value
  *data++ = (currentTemp >> 8);
  *data++ = currentTemp & 0xFF;
  *data++ = humidity_pct;
  *data++ = (rainMm >> 8);
  *data++ = rainMm & 0xFF;
  *data++ = (windMaxMs >> 8);
  *data++ = windMaxMs & 0xFF;
  *data++ = (windAvgMs >> 8);
  *data++ = windAvgMs & 0xFF;
  *data++ = stateAndWdir.byte; //(batteryOk << 5) & windDirState;
  *data++ = (lightLuxTenth >> 8);
  *data++ = lightLuxTenth & 0xFF;
  *data++ = uvIndex;
  *data = rssiDb;

  return 14;
};

// private method for peering, to get and send temperature value only
// TODO: use get() but only send first 2 bytes from buffer?
// uint8_t HBWSIGNALDuino_bresser7in1::get_temp(uint8_t* data) {
//   // MSB first
//   *data++ = (currentTemp >> 8);
//   *data = currentTemp & 0xFF;
//   return 2;
// };


/* main channel loop, called by device loop */
void HBWSIGNALDuino_bresser7in1::loop(HBWDevice* device, uint8_t channel) {
  // check for new message. Copy message directly, else buffer might be overwriten by other sender
  if (hbw_link[hbw_link_pos::MSG_COUNTER] != msgCounter) {
    memset(message_buffer, 0, sizeof(message_buffer));  // make sure it's clean
    memcpy(message_buffer, msg_buffer_ptr, sizeof(message_buffer));
    rssi_raw = hbw_link[hbw_link_pos::RSSI];
    msgCounter = hbw_link[hbw_link_pos::MSG_COUNTER];  // remember counter to not process same msg again
    lastCheck = millis();
    // parse msg directly, on success force to wait min e.g. 5 seconds
    printDebug = true;
  }
  if (millis() - lastCheck >= 500 && printDebug) {  // delay output (check Serial is free?)
    // lastCheck = millis();
    parseMsg(); // TODO: read return code and move parsing to main loop if (parseMsg() == SUCCESS)
    printDebug = false;
  }

  // check for new value, only send on the bus if values changed as set by delta or max send invervall is passed
  if (millis() - lastSend >= 30000) {
    
    uint8_t data[14];
    get(data);
    if (device->sendInfoMessage(channel, sizeof(data), data) != HBWDevice::BUS_BUSY) {
     #ifdef Support_HBWLink_InfoEvent
      // //  uint8_t level[2];
      // //  get_temp(level);
      // // device->sendInfoEvent(channel, 2, level, !NEED_IDLE_BUS);  // send peerings. Info message has just been send, so we send immediately
      // only send temperature value for peerings (first 2 bytes of data[14])
      // device->sendInfoEvent(channel, 2, data, !NEED_IDLE_BUS);  // send peerings. Info message has just been send, so we send immediately
     #endif
    }
    lastSend = millis();
  }
};

/* parse raw message, validate and extract actual values */
uint8_t HBWSIGNALDuino_bresser7in1::parseMsg() {

  /* src: https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_7in1.c */

  int s_type  = message_buffer[6] >> 4;

  // data whitening
  for (unsigned char i = 0; i < sizeof (message_buffer); ++i) {
      message_buffer[i] ^= 0xAA;
  }
 #if defined (HBW_CHANNEL_DEBUG)
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
      EepromPtr->update(eeprom_address_start +1, (uint8_t)(id>>8));  // must match address of condig struct (hbw_config_signalduino_wds_7in1->id)
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
    windMaxMs = wgst_raw;
    windAvgMs = wavg_raw;
    rainMm = rain_raw;
    lightLuxTenth = lght_raw /10;  // max 655,350 sufficent?
    currentTemp = temp_mc *10; // temperature from milli to centi celsius (need factor 100. 2000 == 20.00 °C)
    humidity_pct = humidity;
    uvIndex = uv_raw;
    batteryOk = !battery_low;
    int rssi_tmp = HBWSIGNALDuino_calcRSSI(rssi_raw);
    rssiDb = (rssi_tmp <= 127 || rssi_tmp >= -127) ? rssi_tmp : 0;

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
 #endif
    
    return SUCCESS;
  }

// hbwdebug(F("msg_buff#");
// for (unsigned char i = 0; i < sizeof (message_buffer); ++i) {
//     hbwdebug(F(message_buffer[i], HEX);
// }
#if defined (HBW_CHANNEL_DEBUG)
 Serial.println("ingnored");
#endif
  return MSG_IGNORED;
}


