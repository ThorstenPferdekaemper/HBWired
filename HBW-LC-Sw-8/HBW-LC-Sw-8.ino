//*******************************************************************
//
// HBW-LC-Sw-8
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// - Active HIGH oder LOW kann konfiguriert werden
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v1.01
// - Switch code in "HBWSwitch.h" channel library übertragen. neu: initConfigPins()
// v1.02
// - channel invert angepasst um nach einem Device reset (EEPROM 'gelöscht') keine Invertierung zu haben
// - fix: initConfigPins() in afterReadConfig() geändert.


#define HARDWARE_VERSION 0x00
#define FIRMWARE_VERSION 0x0066
#define HMW_DEVICETYPE 0x83

#define NUM_CHANNELS 8
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x40


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkSwitchSimple.h>
#include <HBWSwitch.h>
#include <HBW_eeprom.h>


// Pins and hardware config
#include "HBW-LC-Sw-8_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_switch switchCfg[NUM_CHANNELS]; // 0x07-0x... ? (address step 2)
} hbwconfig;


// create channel object for the switches
HBWSwitch* switches[NUM_CHANNELS];


class HBSwDevice : public HBWDevice {
    public: 
    HBSwDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
               Stream* _rs485, uint8_t _txen, 
               uint8_t _configSize, void* _config, 
               uint8_t _numChannels, HBWChannel** _channels,
               Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
    HBWDevice(_devicetype, _hardware_version, _firmware_version,
              _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
              _debugstream, linksender, linkreceiver) {
    };
    virtual void afterReadConfig();
};

// device specific defaults
void HBSwDevice::afterReadConfig() {
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
};


HBSwDevice* device = NULL;



void setup()
{
#ifdef USE_HARDWARE_SERIAL
  Serial.begin(19200, SERIAL_8E1);
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin();   // RS485 via SoftwareSerial, uses 19200 baud!
#endif

  // create channels
  static const uint8_t pins[NUM_CHANNELS] = {SWITCH1_PIN, SWITCH2_PIN, SWITCH3_PIN, SWITCH4_PIN, SWITCH5_PIN, SWITCH6_PIN, SWITCH7_PIN, SWITCH8_PIN};
  
  // assing switches (relay) pins to channels
  for(uint8_t i = 0; i < NUM_CHANNELS; i++) {
    switches[i] = new HBWSwitch(pins[i], &(hbwconfig.switchCfg[i]));
  }

  device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
  #ifdef USE_HARDWARE_SERIAL
                         &Serial,
  #else
                         &rs485,
  #endif
                         RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUM_CHANNELS, (HBWChannel**)switches,
  #ifdef USE_HARDWARE_SERIAL  // set _debugstream to NULL
                         NULL,
  #else
                         &Serial,
  #endif
                         NULL, new HBWLinkSwitchSimple<NUM_LINKS, LINKADDRESSSTART>());

  device->setConfigPins(BUTTON, LED);  // 8 and 13 is the default
  
  
#ifndef USE_HARDWARE_SERIAL
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
#endif
}


void loop()
{
  device->loop();
};
