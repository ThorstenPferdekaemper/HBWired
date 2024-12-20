//*******************************************************************
//
// HBW-Sen-EP, RS485 8-channel Puls Counter / S0 interface
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 
// HBW-Sen-EP zum Zählen von elektrischen Pulsen (z.B. S0-Schnittstelle)
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial version
// v0.02
// - rework of channel loop - reading input
// - removed POLLING_TIME config (replaced by fixed debounce time - DEBOUNCE_DELAY set in HBWSenEP.h)


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0002
#define HMW_DEVICETYPE 0x84    //device ID (make sure to import hbw-sen-ep.xml into FHEM)

#define NUMBER_OF_SEN_CHAN 8   // input channels


// HB Wired protocol and modules
#include <HBWired.h>
#include <HBWSenEP.h>
#include <HBW_eeprom.h>


// Pins and hardware config
#include "HBW-Sen-EP_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead


#define NUMBER_OF_CHAN NUMBER_OF_SEN_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_sen_ep SenEpCfg[NUMBER_OF_SEN_CHAN]; // 0x07 - 0x..(address step 9)
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device
HBWDevice* device = NULL;


void setup()
{
  // create channels
  static const uint8_t SenPin[NUMBER_OF_SEN_CHAN] = {Sen1, Sen2, Sen3, Sen4, Sen5, Sen6, Sen7, Sen8};  // assing pins
  
  for(uint8_t i = 0; i < NUMBER_OF_SEN_CHAN; i++) {
    channels[i] = new HBWSenEP(SenPin[i], &(hbwconfig.SenEpCfg[i]));
  }


#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUMBER_OF_CHAN, (HBWChannel**)channels,
                         NULL,
                         NULL, NULL);
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin(19200);   // RS485 via SoftwareSerial, must use 19200 baud!
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUMBER_OF_CHAN, (HBWChannel**)channels,
                         &Serial,
                         NULL, NULL);
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default

  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
#endif
}


void loop()
{
  device->loop();
  POWERSAVE();  // go sleep a bit
};
