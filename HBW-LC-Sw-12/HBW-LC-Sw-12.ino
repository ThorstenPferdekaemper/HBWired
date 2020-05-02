//*******************************************************************
//
// HBW-LC-Sw-12
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 6 + 6 bistabile Relais über Shiftregister und [TODO: 6 Kanal Strommessung]
// 
// - Active HIGH oder LOW kann konfiguriert werden
// - Direktes peering mit Zeitschaltfuntion (on/off delay, on/off time, toggle/toggle to counter)

//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.1
// - initial version
// v0.2
// - Added enhanced peering and basic state machine
// v0.3
// - Added analog inputs
// v0.4
// - Added OE (output enable), to avoid shift register outputs flipping at start up


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0004
#define HMW_DEVICETYPE 0x93

#define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */

#define NUM_SW_CHANNELS 12  // switch/relay
#define NUM_ADC_CHANNELS 6   // analog input
#define NUM_CHANNELS NUM_SW_CHANNELS + NUM_ADC_CHANNELS     // total amount
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x40 // stepping 20



// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkSwitchAdvanced.h>
#include "HBWSwitchSerialAdvanced.h"
#include "HBWAnalogPow.h"


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define RS485_TXEN 2  // Transmit-Enable
  #define shiftReg_OutputEnable 8   // OE output enable, connect to all shift register
  // 6 realys and LED attached to 3 shiftregisters
  #define shiftRegOne_Data  10       //DS serial data input
  #define shiftRegOne_Clock 3       //SH_CP shift register clock input
  #define shiftRegOne_Latch 4       //ST_CP storage register clock input
  // extension shifregister for another 6 relays and LEDs
  #define shiftRegTwo_Data  7
  #define shiftRegTwo_Clock 12
  #define shiftRegTwo_Latch 9
  
#else
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define shiftReg_OutputEnable 12   // OE output enable, connect to all shift register
  // 6 realys and LED attached to 3 shiftregisters
  #define shiftRegOne_Data  10       //DS serial data input
  #define shiftRegOne_Clock 9       //SH_CP shift register clock input
  #define shiftRegOne_Latch 11       //ST_CP storage register clock input
  // extension shifregister for another 6 relays and LEDs
  #define shiftRegTwo_Data  5
  #define shiftRegTwo_Clock 7
  #define shiftRegTwo_Latch 6
  
  #include <FreeRam.h>
  #include <HBWSoftwareSerial.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif

#define LED LED_BUILTIN        // Signal-LED

#define CT_PIN1 A0  // analog input for current transformer, switch channel 1
#define CT_PIN2 A1  // analog input for current transformer, switch channel 2
#define CT_PIN3 A2
#define CT_PIN4 A4
#define CT_PIN5 A5
#define CT_PIN6 A3



struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_switch switchCfg[NUM_SW_CHANNELS]; // 0x07-0x1E (2 bytes each)
  hbw_config_analogPow_in ctCfg[NUM_ADC_CHANNELS];    // 0x1F-0x2A (2 bytes each)
} hbwconfig;


HBWChannel* channels[NUM_CHANNELS];


class HBSwSerDevice : public HBWDevice {
    public:
    HBSwSerDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
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
void HBSwSerDevice::afterReadConfig() {
  if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
};


HBSwSerDevice* device = NULL;

// SHIFT_REGISTER_CLASS defined in "HBWSwitchSerialAdvanced.h"
SHIFT_REGISTER_CLASS* myShReg_one = new SHIFT_REGISTER_CLASS(shiftRegOne_Data, shiftRegOne_Clock, shiftRegOne_Latch);
SHIFT_REGISTER_CLASS* myShReg_two = new SHIFT_REGISTER_CLASS(shiftRegTwo_Data, shiftRegTwo_Clock, shiftRegTwo_Latch);


void setup()
{
  // assing LEDs and switches (relay) pins (i.e. shift register bit position)
  static const uint8_t LEDBitPos[6] = {0, 1, 2, 3, 4, 5};    // shift register 1: 6 LEDs // not only used for the LEDs, also to keep track of the output state! (RAM style)
  static const uint8_t RelayBitPos[6] = { 8, 10, 12,          // shift register 2: 3 relays (with 2 coils each)
                                         16, 18, 20};        // shift register 3: 3 relays (with 2 coils each)
  static const uint8_t currentTransformerPins[6] = {CT_PIN1, CT_PIN2, CT_PIN3, CT_PIN4, CT_PIN5, CT_PIN6};
  
  // create channels
  for(uint8_t i = 0; i < NUM_SW_CHANNELS; i++) {
    if (i < 6) {
      channels[i] = new HBWSwitchSerialAdvanced(RelayBitPos[i], LEDBitPos[i], myShReg_one, &(hbwconfig.switchCfg[i]));
      channels[i+NUM_SW_CHANNELS] = new HBWAnalogPow(currentTransformerPins[i], &(hbwconfig.ctCfg[i]));
    }
    else
      channels[i] = new HBWSwitchSerialAdvanced(RelayBitPos[i %6], LEDBitPos[i %6], myShReg_two, &(hbwconfig.switchCfg[i]));
  };

#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBSwSerDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUM_CHANNELS,(HBWChannel**)channels,
                         NULL,
                         NULL, new HBWLinkSwitchAdvanced<NUM_LINKS, LINKADDRESSSTART>());
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin(19200);   // RS485 via SoftwareSerial, must use 19200 baud!
  
  device = new HBSwSerDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUM_CHANNELS, (HBWChannel**)channels,
                         &Serial,
                         NULL, new HBWLinkSwitchAdvanced<NUM_LINKS, LINKADDRESSSTART>());
   
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default
 
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
#endif

  // set shift register OE pin LOW, this enables the output (only do this after latches have been initialized! OE was held HIGH with pullup during poweron)
  digitalWrite(shiftReg_OutputEnable, LOW);
  pinMode(shiftReg_OutputEnable, OUTPUT);
}


void loop()
{
  device->loop();
};
