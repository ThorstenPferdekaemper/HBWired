//*******************************************************************
//
// HBW-Sen-DB-4
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// Türklingelsensor mit 4 Tastern und Beleuchtung des Klingeltableau
// 
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.10
// - initial version
// v0.20
// - fix for repeatCounter reset handling
// v0.3
// - added buzzer for button feedback (uses tone() builtin function) - needs new XML due to additional config
//   Set pin BUZZER NOT_A_PIN to disable it
// v0.4
// - added peering for backlight chan - needs new XML due to updated config!


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0028
#define HMW_DEVICETYPE 0x98 //device ID (make sure to import hbw-dis-key-4.xml into FHEM)

//TODO? + 1 türsummer?
#define NUMBER_OF_DIM_CHAN 1   // dimmer output (for backlight) - with auto_brightness
#define NUMBER_OF_KEY_CHAN 4   // knobs at your door
#define NUM_LINKS_KEY 20
#define NUM_LINKS_DIM 20
#define LINKADDRESSSTART_DIM 0x40  // any actor peering!
#define LINKADDRESSSTART_KEY 0x13C  // any sensor peering!


// HB Wired protocol and module
#include <HBWired.h>
#include <HBW_eeprom.h>
#include <HBWLinkKey.h>
#include "HBWKeyDoorbell.h"
#include <HBWDimBacklight.h>
#include <HBWLinkSwitchSimple.h>


// Pins and hardware config
#include "HBW-Sen-DB-4_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead


#define NUMBER_OF_CHAN NUMBER_OF_DIM_CHAN + NUMBER_OF_KEY_CHAN

struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_dim_backlight BlDimCfg[NUMBER_OF_DIM_CHAN]; // 0x07 - 0x08 (address step 2)
  hbw_config_key_doorbell DBKeyCfg[NUMBER_OF_KEY_CHAN]; // 0x09 - 0x1C (address step 5)
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device

HBWDevice* device = NULL;


void setup()
{
  channels[0] = new HBWDimBacklight(&(hbwconfig.BlDimCfg[0]), BACKLIGHT_PWM, LDR_PIN);
  
  static const byte BUTTON_PIN[] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4};
  
 #if (NUMBER_OF_KEY_CHAN == 4)
  for(uint8_t i = 0; i < NUMBER_OF_KEY_CHAN; i++) {
    channels[i + NUMBER_OF_DIM_CHAN] = new HBWKeyDoorbell(BUTTON_PIN[i], &(hbwconfig.DBKeyCfg[i]), BUZZER);
  }
 #else
  #error NUMBER_OF_KEY_CHAN channel missmatch!
 #endif
  
 
#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             NULL,
                             new HBWLinkKey<NUM_LINKS_KEY, LINKADDRESSSTART_KEY>(),
                             new HBWLinkSwitchSimple<NUM_LINKS_DIM, LINKADDRESSSTART_DIM>()
                             );
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin();    // RS485 via SoftwareSerial
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
                             new HBWLinkKey<NUM_LINKS_KEY, LINKADDRESSSTART_KEY>(),
                             new HBWLinkSwitchSimple<NUM_LINKS_DIM, LINKADDRESSSTART_DIM>()
                             );
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default

  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F(" #num_chan "));
  //hbwdebug(NUMBER_OF_CHAN);
  hbwdebug(sizeof(channels)/2);

  // calculate EEPORM start addresses. to put in XML
  hbwdebug(F(" #eeStart Chan, Dim:"));
  hbwdebughex(0x07);
  hbwdebug(F(", Key:"));
  hbwdebughex(0x07 + sizeof(hbwconfig.BlDimCfg));
  hbwdebug(F("\n"));
#endif
}


void loop()
{
  device->loop();
  POWERSAVE();  // go sleep a bit
};
