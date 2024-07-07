

#include "HBWSIGNALDuino_bresser7in1.h"
#include "bit_util.h"

#define SENSOR_TYPE_WEATHER   1


// def
// HBWSIGNALDuino_bresser7in1::HBWSIGNALDuino_bresser7in1(uint8_t* _msg_buffer_ptr, uint8_t* _msg_ready_ptr, hbw_config_signalduino_wds_7in1* _config) {
HBWSIGNALDuino_bresser7in1::HBWSIGNALDuino_bresser7in1(uint8_t* _msg_buffer_ptr, uint8_t* _hbw_link, hbw_config_signalduino_wds_7in1* _config) {
   msg_buffer_ptr = _msg_buffer_ptr;
  //  msg_ready = _msg_ready_ptr;
   hbw_link = _hbw_link;
   config = _config;

};


// channel specific settings or defaults
// (This function is called after device read config from EEPROM)
void HBWSIGNALDuino_bresser7in1::afterReadConfig() {
  hbwdebug(F("SIGNALDuino_adv conf - id: "));hbwdebug(config->id);hbwdebug(F("\n"));

}


// void HBWSIGNALDuino_bresser7in1::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
// 	if (config->output_unlocked) {	//0=LOCKED, 1=UNLOCKED
		// foo
// };


uint8_t HBWSIGNALDuino_bresser7in1::get(uint8_t* data) {
	// if (currentState)
		(*data) = 200;
	// else
	// 	(*data) = 0;
	return 1;
};


void HBWSIGNALDuino_bresser7in1::loop(HBWDevice* device, uint8_t channel) {
  // copy new message asap, else buffer might be overwriten by other sender
  // if (*msg_ready) {
  if (hbw_link[0]) {
    memset(message_buffer, 0, sizeof(message_buffer));  // make sure it's clean
    memcpy(message_buffer, msg_buffer_ptr, sizeof(message_buffer));
    rssi = hbw_link[1];
    // *msg_ready = false;
    hbw_link[0] = false;
    lastCheck = millis();
    printDebug = true;
  }
  if (millis() - lastCheck >= 500 && printDebug) {  // delay output (check Serial is free?)
    // lastCheck = millis();
    parseMsg();
    printDebug = false;
  }
  
};

/* parse raw message, validate and extract actual values */
bool HBWSIGNALDuino_bresser7in1::parseMsg() {
// Serial.print("msg_buff#");
// for (unsigned char i = 0; i < sizeof (message_buffer); ++i) {
//     Serial.print(message_buffer[i], HEX);
// }
// Serial.println("#");

  /* src: https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_7in1.c */

  int s_type   = message_buffer[6] >> 4;
  Serial.print("type: ");Serial.println(s_type, HEX);

  // data whitening
  for (unsigned char i = 0; i < sizeof (message_buffer); ++i) {
      message_buffer[i] ^= 0xAA;
  }

  // LFSR-16 digest, generator 0x8810 key 0xba95 final xor 0x6df1
  int chk    = (message_buffer[0] << 8) | message_buffer[1];
  int digest = lfsr_digest16(&message_buffer[2], 23, 0x8810, 0xba95);
  if ((chk ^ digest) != 0x6df1) {
      Serial.println("Digest check failed!");
      return false;
  }

  int id          = (message_buffer[2] << 8) | (message_buffer[3]);
  int flags       = (message_buffer[15] & 0x0f);
  int battery_low = (flags & 0x06) == 0x06;

  if (s_type == SENSOR_TYPE_WEATHER) {
    // if (config->id == 0x0 || config->id == 0xFFFF) {config->id = id; // and update eeprom?};
    // if (id != config->id) return false;
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

    unsigned int light_lux = lght_raw;
    float uv_index = uv_raw * 0.1f;

    // int rssiDb = calcRSSI(rssi);
  Serial.print("id: ");Serial.println(id, HEX);
  Serial.print("batt ok: ");Serial.println(!battery_low);
  Serial.print("wind_dir: ");Serial.println(wdir);
  Serial.print("wind_max_m_s: ");Serial.println(wgst);
  Serial.print("wind_avg_m_s: ");Serial.println(wavg);
  Serial.print("rain: ");Serial.println((uint)rain_raw);
  Serial.print("temp_mc: ");Serial.println(temp_mc);
  Serial.print("humidity: ");Serial.println(humidity);
  Serial.print("light_lux: ");Serial.println(light_lux);
  Serial.print("uv_index: ");Serial.println(uv_index);
  Serial.print("RSSI db: ");Serial.println(calcRSSI(rssi));
    
    return true;
  }

// Serial.print("msg_buff#");
// for (unsigned char i = 0; i < sizeof (message_buffer); ++i) {
//     Serial.print(message_buffer[i], HEX);
// }
 Serial.println("ingnored");
  return false;
}

// calculated RSSI and RSSI value
// https://github.com/Ralf9/SIGNALduinoAdv_FHEM/blob/master/FHEM/00_SIGNALduinoAdv.pm
int HBWSIGNALDuino_bresser7in1::calcRSSI(uint8_t _rssi) {
  return (_rssi>=128 ? ((_rssi-256)/2-74) : (_rssi/2-74));
}

