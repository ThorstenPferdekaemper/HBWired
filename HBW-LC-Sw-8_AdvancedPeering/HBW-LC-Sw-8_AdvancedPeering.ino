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
// v1.03
// - erweitertes peering hinzugefügt (HBWLinkSwitchAdvanced.h, HBWSwitchAdvanced.h) - Benötigt passende XML!


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0067
#define HMW_DEVICETYPE 0x83

#define NUM_CHANNELS 8
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x40


//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


#include <HBWSoftwareSerial.h>
#include <FreeRam.h>


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkSwitchAdvanced.h>
#include <HBWSwitchAdvanced.h>

#define RS485_RXD 4
#define RS485_TXD 2
#define RS485_TXEN 3  // Transmit-Enable

HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX


// Pins
#define BUTTON 8  // Button fuer Factory-Reset etc.
#define LED LED_BUILTIN      // Signal-LED
#define SWITCH1_PIN A0  // Ausgangpins fuer die Relais
#define SWITCH2_PIN A1
#define SWITCH3_PIN A2
#define SWITCH4_PIN A3
#define SWITCH5_PIN A4
#define SWITCH6_PIN A5
#define SWITCH7_PIN 10
#define SWITCH8_PIN 11


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_switch switchCfg[NUM_CHANNELS]; // 0x07-0x... ? (address step 2)
} hbwconfig;


// create channel object for the switches
HBWSwitchAdvanced* switches[NUM_CHANNELS];


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
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 20;
};


HBSwDevice* device = NULL;



void setup()
{
#ifndef NO_DEBUG_OUTPUT
  Serial.begin(115200);  // Serial->USB for debug
#endif
  rs485.begin(19200);   // RS485 via SoftwareSerial, must use 19200 baud!

  // create channels
  uint8_t pins[NUM_CHANNELS] = {SWITCH1_PIN, SWITCH2_PIN, SWITCH3_PIN, SWITCH4_PIN, SWITCH5_PIN, SWITCH6_PIN, SWITCH7_PIN, SWITCH8_PIN};
  
  // assing switches (relay) pins to channels
  for(uint8_t i = 0; i < NUM_CHANNELS; i++) {
    switches[i] = new HBWSwitchAdvanced(pins[i], &(hbwconfig.switchCfg[i]));
  }

  device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485,RS485_TXEN,sizeof(hbwconfig),&hbwconfig,
                         NUM_CHANNELS,(HBWChannel**)switches,
  #ifdef NO_DEBUG_OUTPUT
                         NULL,
  #else
                         &Serial,
  #endif
                         NULL, new HBWLinkSwitchAdvanced(NUM_LINKS,LINKADDRESSSTART));

  device->setConfigPins(BUTTON, LED);  // 8 and 13 is the default
  
  
#ifndef NO_DEBUG_OUTPUT
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
#endif
}


void loop()
{
  device->loop();
};
