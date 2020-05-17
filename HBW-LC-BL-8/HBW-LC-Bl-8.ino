//*******************************************************************
//
// HBW-LC-Bl-8
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 8 Kanal Rollosteuerung
// - Direktes Peering möglich. (open: 100%, close: 0%, toggle/stop, set level)
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.1
// - initial port to new library
// - logging mechanism changed, only send info message once blind has reached final postition
// - time measurement changed to duration, to avoid millis() rollover issue
// - added additional pause, when changing direction
// - added basic direct peering
// - changed to USART instead of "HBWSoftwareSerial" to free up IO ports, sacrificed debugging (use HBW-LC-Bl-4 as test device)
// v0.2
// - added level option in peering to set target value
// v0.3
// - added one analog channel for bus voltage measurement
// v0.31
// - more configuration options for analog channel (need new XML)


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x001F
#define HMW_DEVICETYPE 0x92 //BL8 device (make sure to import hbw_lc_bl-8.xml into FHEM)

#define NUMBER_OF_BLINDS 8
#define NUM_LINKS 64
#define LINKADDRESSSTART 0x48   // address step 9
#define NUMBER_OF_ANALOG_CHAN 1  // analog input channel (bus voltage)


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWBlind.h>
#include <HBWLinkBlindSimple.h>
#include <HBWAnalogIn.h>

// Pins
#define RS485_TXEN 2  // Transmit-Enable

#define BUTTON A6  // Button fuer Factory-Reset etc.
#define LED LED_BUILTIN        // Signal-LED

#define BLIND1_ACT 10		// "Ein-/Aus-Relais"
#define BLIND1_DIR 9		// "Richungs-Relais"
#define BLIND2_ACT 8
#define BLIND2_DIR 7
#define BLIND3_ACT 6
#define BLIND3_DIR 5
#define BLIND4_ACT 4
#define BLIND4_DIR 3

#define BLIND5_ACT A5
#define BLIND5_DIR A4
#define BLIND6_ACT A3
#define BLIND6_DIR A2
#define BLIND7_ACT A0
#define BLIND7_DIR A1
#define BLIND8_ACT 12
#define BLIND8_DIR 11

#define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage (using internal 1.1 volt reference)

/*>>> more config in HBWBlind.h <<<*/

#define NUMBER_OF_CHAN NUMBER_OF_BLINDS + NUMBER_OF_ANALOG_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_blind blindCfg[NUMBER_OF_BLINDS]; // 0x07-0x3E (address step 7)
  hbw_config_analog_in adcInCfg[NUMBER_OF_ANALOG_CHAN]; // 0x3F - 0x40 (address step 2)
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
  analogReference(INTERNAL);    // select internal 1.1 volt reference (to measure external bus voltage)
  Serial.begin(19200, SERIAL_8E1); // RS485 via UART Serial

  // assing relay pins
  static const uint8_t blindDir[8] = {BLIND1_DIR, BLIND2_DIR, BLIND3_DIR, BLIND4_DIR, BLIND5_DIR, BLIND6_DIR, BLIND7_DIR, BLIND8_DIR};
  static const uint8_t blindAct[8] = {BLIND1_ACT, BLIND2_ACT, BLIND3_ACT, BLIND4_ACT, BLIND5_ACT, BLIND6_ACT, BLIND7_ACT, BLIND8_ACT};
  
  // create channels
  for(uint8_t i = 0; i < NUMBER_OF_BLINDS; i++) {
    channels[i] = new HBWChanBl(blindDir[i], blindAct[i], &(hbwconfig.blindCfg[i]));
  }
#if NUMBER_OF_ANALOG_CHAN == 1
  channels[NUMBER_OF_CHAN -1] = new HBWAnalogIn(ADC_BUS_VOLTAGE, &(hbwconfig.adcInCfg[0]));
#endif

  device = new HBBlDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &Serial, RS485_TXEN,sizeof(hbwconfig),&hbwconfig,
                         NUMBER_OF_CHAN,(HBWChannel**)channels,
                         NULL,
                         NULL, new HBWLinkBlindSimple<NUM_LINKS, LINKADDRESSSTART>());
  
  device->setConfigPins(BUTTON, LED);
  device->setStatusLEDPins(LED, LED); // Tx, Rx LEDs using config LED
}


void loop()
{
  device->loop();
};
