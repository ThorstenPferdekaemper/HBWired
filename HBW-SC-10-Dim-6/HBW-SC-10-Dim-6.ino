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
// v0.1
// - initial version
// v0.2
// - added OFFDELAY_BLINK, OFFDELAY_STEP peering setting
// - clean-up
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
//

// TODO: Implement dim peering params: RAMP_START_STEP. Validate behaviour of OnLevelPrio and on/off time 'minimal' vs offLevel, etc.
// TODO: reduce RAM usage! (with the current amount of channels, no additional features possible) - or use larger microcontroller...


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x003C
#define HMW_DEVICETYPE 0x96 //device ID (make sure to import hbw_io-10_dim-6.xml into FHEM)

#define NUMBER_OF_INPUT_CHAN 10   // input channel - pushbutton, key, other digital in
#define NUMBER_OF_SEN_INPUT_CHAN 10  // equal number of sensor channels, using same ports/IOs as INPUT_CHAN
#define NUMBER_OF_DIM_CHAN 6  // PWM & analog output channels
#define NUMBER_OF_VIRTUAL_KEY_CHAN 6  // virtual keys, mapped to dimmer channel

#define NUM_LINKS_DIM 20    // address step 42
#define LINKADDRESSSTART_DIM 0x038   // ends @0x37F
#define NUM_LINKS_INPUT 20    // address step 6
#define LINKADDRESSSTART_INPUT 0x380   // ends @0x3F7


//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkDimmerAdvanced.h>
#include <HBWDimmerAdvanced.h>
#include <HBWLinkKey.h>
#include <HBWKey.h>
#include <HBWSenSC.h>
//#include "HBWKeyVirtual.h"
#include "HBWDimmerVirtual.h"

// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  
  #define PWM1 9  // PWM out (controlled by timer1)
  #define PWM2_DAC 5  // PWM out (controlled by timer0)
  #define PWM3_DAC 10  // PWM out (controlled by timer1)
  #define PWM4   6 // PWM out (controlled by timer0)
  #define PWM5 11  // PWM out (controlled by timer2)
  #define PWM6 3  // PWM out (controlled by timer2)

  #define IO1 A3
  #define IO2 A2
  #define IO3 A1
  #define IO4 A0
  #define IO5 4
  #define IO6 7
  #define IO7 8
  #define IO8 12
  #define IO9 A4
  #define IO10 A5
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.

  #define PWM1 9
  #define PWM2_DAC 5
  #define PWM3_DAC 10
  #define PWM4 6
  #define PWM5 11
  #define PWM6 NOT_A_PIN  // dummy pin to fill the array elements

  #define IO1 7
  #define IO2 A5
  #define IO3 12
  #define IO4 A0
  #define IO5 A1
  #define IO6 A2
  #define IO7 A3
  #define IO8 A4
  #define IO9 NOT_A_PIN  // dummy pin to fill the array elements
  #define IO10 NOT_A_PIN  // dummy pin to fill the array elements

  #include "FreeRam.h"
  #include <HBWSoftwareSerial.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL

#define LED LED_BUILTIN        // Signal-LED

#define NUMBER_OF_CHAN NUMBER_OF_DIM_CHAN + NUMBER_OF_INPUT_CHAN + NUMBER_OF_SEN_INPUT_CHAN + NUMBER_OF_VIRTUAL_KEY_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_dim dimCfg[NUMBER_OF_DIM_CHAN]; // 0x07 - 0x12 (address step 2)
  hbw_config_senSC senCfg[NUMBER_OF_SEN_INPUT_CHAN]; // 0x13 - 0x1C (address step 1)
  hbw_config_key keyCfg[NUMBER_OF_INPUT_CHAN]; // 0x1D - 0x30 (address step 2)
  //hbw_config_key_virt keyVirtCfg[NUMBER_OF_VIRTUAL_KEY_CHAN]; // 0x31 - 0x37 (address step 1)
  hbw_config_dim_virt dimVirtCfg[NUMBER_OF_VIRTUAL_KEY_CHAN]; // 0x31 - 0x37 (address step 1)
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
  //change from fast-PWM to phase-correct PWM
  // (This is the timer0 controlled PWM module. Do not change prescaler, it would impact millis() & delay() functions.)
  //TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM00);
//  TCCR0A = B00000001; // phase-correct PWM @490Hz
// TODO: fixme - not working! millis() is running two times slower when not in fast-PWM! - interrupt 'issue'
  
  setupPwmTimer1();
  setupPwmTimer2();
  
  // create channels
#if NUMBER_OF_DIM_CHAN == 6
  static const uint8_t PWMOut[6] = {PWM1, PWM2_DAC, PWM3_DAC, PWM4, PWM5, PWM6};  // assing pins
  
  // dimmer + dimmer key channels
  for(uint8_t i = 0; i < NUMBER_OF_DIM_CHAN; i++) {
    channels[i] = new HBWDimmerAdvanced(PWMOut[i], &(hbwconfig.dimCfg[i]));
    //channels[i + NUMBER_OF_DIM_CHAN + NUMBER_OF_INPUT_CHAN + NUMBER_OF_SEN_INPUT_CHAN] = new HBWKeyVirtual(i, &(hbwconfig.keyVirtCfg[i]));
    channels[i + NUMBER_OF_DIM_CHAN + NUMBER_OF_INPUT_CHAN + NUMBER_OF_SEN_INPUT_CHAN] = new HBWDimmerVirtual(channels[i], &(hbwconfig.dimVirtCfg[i]));
  };
#else
  #error Dimming channel count and pin missmatch!
#endif

#if NUMBER_OF_INPUT_CHAN == 10 && NUMBER_OF_SEN_INPUT_CHAN == 10
  static const uint8_t digitalInput[10] = {IO1, IO2, IO3, IO4, IO5, IO6, IO7, IO8, IO9, IO10};  // assing pins

  // input sensor and key channels
  for(uint8_t i = 0; i < NUMBER_OF_SEN_INPUT_CHAN; i++) {
    channels[i + NUMBER_OF_DIM_CHAN] = new HBWSenSC(digitalInput[i], &(hbwconfig.senCfg[i]));
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
