//*******************************************************************
//
// HBW-SC-10-Dim-6
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// PWM/0-10V/1-10V Master Dimmer + 10 digitale Eingänge (Key/Taster & Sensor) & Sensorkontakte
// - Direktes Peering für Dimmer möglich. (HBWLinkDimmerAdvanced)
// - Direktes Peering für Taster möglich. (HBWLinkKey)
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.3
// - added input channels: Key & Sensor
// v0.4
// - added virtual key channels, allow external peering with Dim channels (e.g. switch 1-10V dimmer on/off)
// v0.5
// - added state flags
// - put key channels back in, kept virtual key channels (dimmer key)
// v0.52
// - fixed state flags, 'minimal' on/off time and added OnLevelPrio for on time
// v0.6
// - replaced virtual key by virtual dimmer channels (HBWDimmerVirtual) - needs new XML
// v0.7
// - internal rework of state engine (using https://github.com/pa-pa/AskSinPP)
// - fixed onLevel prio, added RAMP_START_STEP peering parameter handling
// v0.8
// - HBWDimmerAdvanced/HBWLinkDimmerAdvanced BREAKING CHANGE: aligned peering parameter (jump table at the very end) - same as switchAdv.
//   Saves RAM, but requires updating (or recreation) all DIMMER & vDim peerings


// TODO: Validate behaviour of OnLevelPrio and on/off time 'minimal' vs offLevel, etc.
// TODO: RAM usage is high... with the current amount of channels, no additional features possible - like HBWKeyVirtual (use larger microcontroller...)


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0051
#define HMW_DEVICETYPE 0x96 //device ID (make sure to import hbw_io-10_dim-6.xml into FHEM)

#define NUMBER_OF_INPUT_CHAN 10   // input channel - pushbutton, key, other digital in
#define NUMBER_OF_SEN_INPUT_CHAN 10  // equal number of sensor channels, using same ports/IOs as INPUT_CHAN
#define NUMBER_OF_DIM_CHAN 6  // PWM & analog output channels
//#define NUMBER_OF_VIRTUAL_KEY_CHAN 6  // virtual key, mapped to dimmer channel
#define NUMBER_OF_VIRTUAL_DIM_CHAN 6  // virtual dimmer, mapped to dimmer channel

#define NUM_LINKS_DIM 20    // address step 42
#define LINKADDRESSSTART_DIM 0x038   // ends @0x37F
#define NUM_LINKS_INPUT 20    // address step 6
#define LINKADDRESSSTART_INPUT 0x380   // ends @0x3F7


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkDimmerAdvanced.h>
#include <HBWDimmerAdvanced.h>
#include <HBWLinkKey.h>
#include <HBWKey.h>
#include <HBWSenSC.h>
//#include "HBWKeyVirtual.h"
#include "HBWDimmerVirtual.h"
#include <HBW_eeprom.h>


// Pins and hardware config
#include "HBW-SC-10-Dim-6_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead


#define NUMBER_OF_CHAN NUMBER_OF_DIM_CHAN + NUMBER_OF_INPUT_CHAN + NUMBER_OF_SEN_INPUT_CHAN + NUMBER_OF_VIRTUAL_DIM_CHAN //+ NUMBER_OF_VIRTUAL_KEY_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_dim dimCfg[NUMBER_OF_DIM_CHAN]; // 0x07 - 0x12 (address step 2)
  hbw_config_senSC senCfg[NUMBER_OF_SEN_INPUT_CHAN]; // 0x13 - 0x1C (address step 1)
  hbw_config_key keyCfg[NUMBER_OF_INPUT_CHAN]; // 0x1D - 0x30 (address step 2)
  hbw_config_dim_virt dimVirtCfg[NUMBER_OF_VIRTUAL_DIM_CHAN]; // 0x31 - 0x37 (address step 1)
  // hbw_config_key_virt keyVirtCfg[NUMBER_OF_VIRTUAL_KEY_CHAN]; // 0x38 - 0x3D (address step 1)
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device


class HBDimIODevice : public HBWDevice {
    public:
    HBDimIODevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
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
void HBDimIODevice::afterReadConfig() {
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
};

HBDimIODevice* device = NULL;



void setup()
{
  SetupHardware();  // special hardware related settings. See config_...h. Function can be empty, when not needed
  
  // create channels
 #if NUMBER_OF_DIM_CHAN == 6
  static const uint8_t PWMOut[6] = {PWM1, PWM2_DAC, PWM3_DAC, PWM4, PWM5, PWM6};  // assing pins
  
  // dimmer + dimmer vKey/vDim channels
  for(uint8_t i = 0; i < NUMBER_OF_DIM_CHAN; i++) {
    channels[i] = new HBWDimmerAdvanced(PWMOut[i], &(hbwconfig.dimCfg[i]));
    channels[i + NUMBER_OF_DIM_CHAN + NUMBER_OF_INPUT_CHAN + NUMBER_OF_SEN_INPUT_CHAN] = new HBWDimmerVirtual(channels[i], &(hbwconfig.dimVirtCfg[i]));
    // channels[i + NUMBER_OF_DIM_CHAN + NUMBER_OF_INPUT_CHAN + NUMBER_OF_SEN_INPUT_CHAN + NUMBER_OF_VIRTUAL_DIM_CHAN] = new HBWKeyVirtual(i, &(hbwconfig.keyVirtCfg[i]));
  };
 #else
  #error Dimming channel count and pin missmatch!
 #endif

 #if NUMBER_OF_INPUT_CHAN == 10 && NUMBER_OF_SEN_INPUT_CHAN == 10
  static const uint8_t digitalInput[10] = {IO1, IO2, IO3, IO4, IO5, IO6, IO7, IO8, IO9, IO10};  // assing pins

  // input sensor and key channels
  for(uint8_t i = 0; i < NUMBER_OF_SEN_INPUT_CHAN; i++) {
    channels[i + NUMBER_OF_DIM_CHAN] = new HBWSenSC(digitalInput[i], &(hbwconfig.senCfg[i]), true);
    channels[i + NUMBER_OF_DIM_CHAN + NUMBER_OF_SEN_INPUT_CHAN] = new HBWKey(digitalInput[i], &(hbwconfig.keyCfg[i]));
  };
 #else
  #error Input channel count and pin missmatch!
 #endif


 #ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBDimIODevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             NULL,
                             new HBWLinkKey<NUM_LINKS_INPUT, LINKADDRESSSTART_INPUT>(), new HBWLinkDimmerAdvanced<NUM_LINKS_DIM, LINKADDRESSSTART_DIM>());
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
 #else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin(19200);   // RS485 via SoftwareSerial, must use 19200 baud!
  
  device = new HBDimIODevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
                             new HBWLinkKey<NUM_LINKS_INPUT, LINKADDRESSSTART_INPUT>(), new HBWLinkDimmerAdvanced<NUM_LINKS_DIM, LINKADDRESSSTART_DIM>());
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default
  //device->setStatusLEDPins(LED, LED); // Tx, Rx LEDs

  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
  
  // show timer register for debug purpose
//  hbwdebug(F("TCCR0A: "));
//  hbwdebug(TCCR0A);
//  hbwdebug(F("\n"));
//  hbwdebug(F("TCCR0B: "));
//  hbwdebug(TCCR0B);
//  hbwdebug(F("\n"));
 #endif
}


void loop()
{
  device->loop();
  POWERSAVE();  // go sleep a bit
};
