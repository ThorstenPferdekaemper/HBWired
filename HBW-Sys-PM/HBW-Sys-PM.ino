//*******************************************************************
//
// HBW-Sys-PM (-sw4-in2)
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// - Read / monitor Bus Voltage, Current & Power using INA236 chip
// - Shows status and config for alarming (over power, over/under voltage, ...) with direct peering (key)
// - Additional outputs (switch) and analog inputs
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.1
// - initial creation


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0007
#define HMW_DEVICETYPE 0x8D

#define NUM_SW_CHANNELS 4
#define NUM_IN_CHANNELS 2
#define NUM_PM_CHAN 1    // power measure
// #define NUM_VIRT_CHAN 3  // virtual key channels for optional alarm trigger // TODO: not working like this. check if this is really useful...
#define NUM_LINKS_ACTOR 24
#define LINKADDRESSSTART_ACTOR 0x60 // start of actor peerings (switch)
#define NUM_LINKS_SENSOR 20
#define LINKADDRESSSTART_SENSOR 0x2CC  // start addr of all sensor peerings (same address_step!!)


// HB Wired protocol and modules
#include <HBWired.h>
#include <HBWLinkSwitchAdvanced.h>
#include <HBWSwitchAdvanced.h>
// #include <HBWKeyVirtual.h>
#include <HBWLinkKey.h>
#include <HBWAnalogIn.h>
#include "HBWPMeasure.h"


// Pins and hardware config
#include "HBW-Sys-PM_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_power_measure pMeasureCfg[NUM_PM_CHAN]; // address step 16, start 0x07
  hbw_config_switch switchCfg[NUM_SW_CHANNELS]; // address step 2, start 0x17
  hbw_config_analog_in analogInCfg[NUM_IN_CHANNELS]; // step 6, start 0x1F
  // hbw_config_key_virt keyVirtCfg[NUM_VIRT_CHAN]; // step 1, start 0x1F
} hbwconfig;

// calculate total amount of channels
#define NUM_CHANNELS NUM_PM_CHAN + NUM_SW_CHANNELS + NUM_IN_CHANNELS//+ NUM_VIRT_CHAN

// create channel object for the switches
HBWChannel* channels[NUM_CHANNELS];


class HBPmDevice : public HBWDevice {
    public: 
    HBPmDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
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
void HBPmDevice::afterReadConfig() {
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;  // logging/feedback for actor channels
};


HBPmDevice* device = NULL;


void setup()
{
#ifdef USE_HARDWARE_SERIAL
  Serial.begin(19200, SERIAL_8E1);
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin();   // RS485 via SoftwareSerial, uses 19200 baud!
#endif

  // create channels
  channels[0] = new HBWPMeasure(&(hbwconfig.pMeasureCfg[0]), &INA236, &Wire);
  // channels[1] = new HBWKeyVirtual(0, &(hbwconfig.keyVirtCfg[0]));
  // for(uint8_t i = 0; i < NUM_VIRT_CHAN; i++) {
  //   // channels[i +NUM_PM_CHAN] = new HBWPMeasure(&(hbwconfig.pMeasureAlarmCfg[i]));
  //   channels[i +NUM_PM_CHAN] = new HBWKeyVirtual(255, &(hbwconfig.keyVirtCfg[i]));
  // }

  // if there is more than one channel of a kind, create them in a loop...
  static const uint8_t pins_sw[NUM_SW_CHANNELS] = {SWITCH1_PIN, SWITCH2_PIN, SWITCH3_PIN, SWITCH4_PIN};
  static const uint8_t pins_input[NUM_IN_CHANNELS] = {INPUT1_PIN, INPUT2_PIN};
  
  for (uint8_t i = 0; i < NUM_SW_CHANNELS; i++) {
    channels[i +NUM_PM_CHAN] = new HBWSwitchAdvanced(pins_sw[i], &(hbwconfig.switchCfg[i]));
    if (i < NUM_IN_CHANNELS)
      channels[i +NUM_PM_CHAN +NUM_SW_CHANNELS] = new HBWAnalogIn(pins_input[i], &(hbwconfig.analogInCfg[i]));
  }


  device = new HBPmDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
  #ifdef USE_HARDWARE_SERIAL
                         &Serial,
  #else
                         &rs485,
  #endif
                         RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUM_CHANNELS, (HBWChannel**)channels,
  #ifdef USE_HARDWARE_SERIAL  // set _debugstream to NULL
                         NULL,
  #else
                         &Serial,
  #endif
                         new HBWLinkKey<NUM_LINKS_SENSOR, LINKADDRESSSTART_SENSOR>(),
                         new HBWLinkSwitchAdvanced<NUM_LINKS_ACTOR, LINKADDRESSSTART_ACTOR>()
                         );

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
  POWERSAVE();  // go sleep a bit
};
