//*******************************************************************
//
// HBW-LC-Bl-4
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 4 Kanal Rollosteuerung
// - Direktes Peering möglich. (open: 100%, close: 0%, toggle/stop, set level)
//
// Vorlage: https://github.com/loetmeister/HM485-Lib/tree/markus/HBW-LC-Bl4
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.1
// - initial port to new library
// v0.2
// - logging mechanism changed, only send info message once blind has reached final postition
// v0.3
// - time measurement changed to duration, to avoid millis() rollover issue
// - added additional pause, when changing direction
// - added basic direct peering
// v0.31
// - fixed peering for long press
// v0.4
// - added level option in peering to set target value
// v0.5
// - added one analog channel for bus voltage measurement
// v0.51
// - more configuration options for analog channel (need new XML)
// v0.52
// - added idle powersave
// v0.6
// - added config option MotorDelay (MOTOR_STARTUP_DELAY)
// v0.65
// - implemented REFERENCE_RUN_COUNTER config option
// v0.66
// - MotorDelay (MOTOR_STARTUP_DELAY) fix
// - Position on device reset can be set with BL_POS_UNKNOWN in HBWBlind.h. New value 1 == 0.5% (not 0% anymore)
// - Fixed/switched UP/DOWN (UP=100%)
// v0.7
// - fixed overflow of runtime occuring over 32.8 seconds


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0046
#define HMW_DEVICETYPE 0x82 //BL4 device (make sure to import hbw_lc_bl-4.xml into FHEM)

#define NUMBER_OF_BLINDS 4
#define NUM_LINKS 32
#define LINKADDRESSSTART 0x48   // address step 9
#define NUMBER_OF_ANALOG_CHAN 1  // analog input channel (bus voltage)


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkBlindSimple.h>
#include <HBWBlind.h>
#include <HBWAnalogIn.h>
#include <HBW_eeprom.h>


// Pins and hardware config
#include "HBW-LC-BL-4_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead


/*>>> more config in HBWBlind.h <<<*/

#define NUMBER_OF_CHAN NUMBER_OF_BLINDS + NUMBER_OF_ANALOG_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_blind blindCfg[NUMBER_OF_BLINDS]; // 0x07-0x22 (address step 7)
  hbw_config_analog_in adcInCfg[NUMBER_OF_ANALOG_CHAN]; // 0x23 - 0x28 (address step 6)
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];


class HBBlDevice : public HBWDevice {
    public: 
    HBBlDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
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
void HBBlDevice::afterReadConfig() {
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
};

HBBlDevice* device = NULL;



void setup()
{
  SetupHardware();
  
#ifdef USE_HARDWARE_SERIAL
  Serial.begin(19200, SERIAL_8E1);
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin();   // RS485 via SoftwareSerial, uses 19200 baud!
#endif

  // create channels
  static const uint8_t blindDir[4] = {BLIND1_DIR, BLIND2_DIR, BLIND3_DIR, BLIND4_DIR};
  static const uint8_t blindAct[4] = {BLIND1_ACT, BLIND2_ACT, BLIND3_ACT, BLIND4_ACT};
  
  // assing relay pins
  for(uint8_t i = 0; i < NUMBER_OF_BLINDS; i++) {
    channels[i] = new HBWChanBl(blindDir[i], blindAct[i], &(hbwconfig.blindCfg[i]));
  }
#if NUMBER_OF_ANALOG_CHAN == 1
  channels[NUMBER_OF_CHAN -1] = new HBWAnalogIn(ADC_BUS_VOLTAGE, &(hbwconfig.adcInCfg[0]));
#endif

  device = new HBBlDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
  #ifdef USE_HARDWARE_SERIAL
                         &Serial,
  #else
                         &rs485,
  #endif
                         RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUMBER_OF_CHAN, (HBWChannel**)channels,
  #ifdef USE_HARDWARE_SERIAL  // set _debugstream to NULL
                         NULL,
  #else
                         &Serial,
  #endif
                         NULL, new HBWLinkBlindSimple<NUM_LINKS, LINKADDRESSSTART>());
   
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
