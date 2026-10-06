//*******************************************************************
//
// HBW-HBW-Generic, template
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 
// HBW-Generic: Setzten der Geräteadresse (OWN_ADDRESS) und Vorlage
// für neue Geräte
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial version


#define HARDWARE_VERSION 0x00
#define FIRMWARE_VERSION 0x0001
#define HMW_DEVICETYPE 0xFF    // temp device ID (make sure to import hbw-generic.xml into FHEM)


// HB Wired protocol and modules
#include <HBWired.h>
#include <HBW_eeprom.h>


// Pins and hardware config
#include "HBW-Generic_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7

} hbwconfig;

#define NUMBER_OF_CHAN 1  // need minimum one channel for a device
HBWChannel* channels[NUMBER_OF_CHAN];
HBWDevice* device = NULL;


void setup()
{
  // create channels
  // static const uint8_t Pin[NUMBER_OF_SEN_CHAN] = {Sen1, Sen2, Sen3, Sen4, Sen5, Sen6, Sen7, Sen8};  // assing pins
  
  // for(uint8_t i = 0; i < NUMBER_OF_SEN_CHAN; i++) {
  //   channels[i] = new HBWSenEP(Pin[i], &(hbwconfig.SenEpCfg[i]));
  // }
  channels[0] = new HBWChannel();  // dummy channel


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
