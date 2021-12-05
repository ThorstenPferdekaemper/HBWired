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
// v1.04
// - state flag added


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0069
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


// Pins
#define LED LED_BUILTIN      // Signal-LED

#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  
  #define SWITCH1_PIN A4  // Ausgangpins fuer die Relais
  #define SWITCH2_PIN A2
  #define SWITCH3_PIN A0
  #define SWITCH4_PIN 10
  #define SWITCH5_PIN A1
  #define SWITCH6_PIN 9
  #define SWITCH7_PIN A3
  #define SWITCH8_PIN 5
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.

  #define SWITCH1_PIN A0  // Ausgangpins fuer die Relais
  #define SWITCH2_PIN A1
  #define SWITCH3_PIN A2
  #define SWITCH4_PIN A3
  #define SWITCH5_PIN A4
  #define SWITCH6_PIN A5
  #define SWITCH7_PIN 10
  #define SWITCH8_PIN 11
  
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif



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
    switches[i] = new HBWSwitchAdvanced(pins[i], &(hbwconfig.switchCfg[i]));
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
                         NULL, new HBWLinkSwitchAdvanced<NUM_LINKS, LINKADDRESSSTART>());

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
