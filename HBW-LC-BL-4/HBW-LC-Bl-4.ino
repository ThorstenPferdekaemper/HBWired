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


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0028

#define NUMBER_OF_BLINDS 4
#define NUM_LINKS 32
#define LINKADDRESSSTART 0x26

#define HMW_DEVICETYPE 0x82 //BL4 device (make sure to import hbw_lc_bl-4.xml into FHEM)


//#define NO_DEBUG_OUTPUT   // disable debug output on serial/USB

#include <HBWSoftwareSerial.h>
#include <FreeRam.h>

// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkBlindSimple.h>
#include <HBWBlind.h>

#define RS485_RXD 4
#define RS485_TXD 2
#define RS485_TXEN 3  // Transmit-Enable

// HBWSoftwareSerial can only do 19200 baud
HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX


// Pins
#define BUTTON 8  // Button fuer Factory-Reset etc.
#define LED LED_BUILTIN        // Signal-LED

#define BLIND1_ACT 5		// "Ein-/Aus-Relais"
#define BLIND1_DIR 6		// "Richungs-Relais"
#define BLIND2_ACT A0
#define BLIND2_DIR A1
#define BLIND3_ACT A2
#define BLIND3_DIR A3
#define BLIND4_ACT A4
#define BLIND4_DIR A5

/*>>> more config in HBWBlind.h <<<*/


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_blind blindCfg[NUMBER_OF_BLINDS]; // 0x07-0x... ? (step 7)
} hbwconfig;


HBWChanBl* blinds[NUMBER_OF_BLINDS];


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
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 20;
};

HBBlDevice* device = NULL;



void setup()
{
#ifndef NO_DEBUG_OUTPUT
  Serial.begin(19200);
#endif
  rs485.begin();    // RS485 via SoftwareSerial

  // create channels
  byte blindDir[4] = {BLIND1_DIR, BLIND2_DIR, BLIND3_DIR, BLIND4_DIR};
  byte blindAct[4] = {BLIND1_ACT, BLIND2_ACT, BLIND3_ACT, BLIND4_ACT};
  
  // assing relay pins
  for(uint8_t i = 0; i < NUMBER_OF_BLINDS; i++) {
    blinds[i] = new HBWChanBl(blindDir[i], blindAct[i], &(hbwconfig.blindCfg[i]));
  }

  device = new HBBlDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485,RS485_TXEN,sizeof(hbwconfig),&hbwconfig,
                         NUMBER_OF_BLINDS,(HBWChannel**)blinds,
  #ifdef NO_DEBUG_OUTPUT
                         NULL,
  #else
                         &Serial,
  #endif
                         NULL, new HBWLinkBlindSimple(NUM_LINKS,LINKADDRESSSTART));
   
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

