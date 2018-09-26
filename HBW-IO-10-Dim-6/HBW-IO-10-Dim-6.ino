//*******************************************************************
//
// HBW-IO-10-Dim-6
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// PWM/0-10V/1-10V Master Dimmer + 10 digitale Eingänge (Sensor)
// - Direktes Peering für Dimmer möglich. (HBWLinkDimmerAdvanced)
// - Direktes Peering für Taster möglich. (HBWLinkKey)
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial version
// v0.02
// - added OFFDELAY_BLINK, OFFDELAY_STEP peering setting
// - clean-up
// v0.03
// - added input channels: Key & Sensor
//
// TODO: Implement dim peering params: ON_LEVEL_PRIO for onTime, RAMP_START_STEP


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0003

#define NUMBER_OF_INPUT_CHAN 10   // input channel - pushbutton, key, other digital in
#define NUMBER_OF_SEN_INPUT_CHAN 10  // same number of sensor channels, mapped to INPUT_CHAN
#define NUMBER_OF_DIM_CHAN 6  // PWM & analog output channels

#define NUM_LINKS_DIM 20    // address step 42
#define LINKADDRESSSTART_DIM 0x038   // ends @0x37F
#define NUM_LINKS_INPUT 20    // address step 6
#define LINKADDRESSSTART_INPUT 0x380   // ends @0x3F7

#define HMW_DEVICETYPE 0x96 //device ID (make sure to import hbw_io-10_dim-6.xml into FHEM)

//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) - this disables debug output


#include "FreeRam.h"

// HB Wired protocol and module
#include <HBWired.h>
#include "HBWLinkDimmerAdvanced.h"
#include "HBWDimmerAdvanced.h"
#include <HBWLinkKey.h>
#include <HBWKey.h>
#include "HBWSenSC.h"

// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  
  #define PWM1 3  // PWM out (controlled by timer2)
  #define PWM2_DAC 5  // PWM out (controlled by timer0)
  #define PWM3_DAC 6  // PWM out (controlled by timer0)
  #define PWM4 9  // PWM out (controlled by timer1)
  #define PWM5 10  // PWM out (controlled by timer1)
  #define PWM6 11  // PWM out (controlled by timer2)

  #define IO1 4
  #define IO2 7
  #define IO3 8
  #define IO4 12
  #define ADC1 A0
  #define ADC2 A1
  #define ADC3 A2
  #define ADC4 A3
  #define ADC5 A4
  #define ADC6 A5
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.

  #define PWM1 NOT_A_PIN  // dummy pin to fill the array elements
  #define PWM2_DAC 5
  #define PWM3_DAC 6
  #define PWM4 9
  #define PWM5 10
  #define PWM6 11

  #define IO1 4
  #define IO2 7
  #define IO3 A5
  #define IO4 12
  #define ADC1 A0
  #define ADC2 A1
  #define ADC3 A2
  #define ADC4 A3
  #define ADC5 A4
  #define ADC6 NOT_A_PIN  // dummy pin to fill the array elements
  
  #include "HBWSoftwareSerial.h"
  // HBWSoftwareSerial can only do 19200 baud
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL

#define LED LED_BUILTIN        // Signal-LED

#define NUMBER_OF_DIM NUMBER_OF_DIM_DAC + NUMBER_OF_DIM_PWM
#define NUMBER_OF_CHAN NUMBER_OF_DIM_CHAN + NUMBER_OF_INPUT_CHAN + NUMBER_OF_SEN_INPUT_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_dim dimCfg[NUMBER_OF_DIM_CHAN]; // 0x07 - 0x12 (address step 2)
  hbw_config_senSC senCfg[NUMBER_OF_SEN_INPUT_CHAN]; // 0x13 - 0x1C (address step 1)
  hbw_config_key keyCfg[NUMBER_OF_INPUT_CHAN]; // 0x1D - 0x30 (address step 2)
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

  byte PWMOut[6] = {PWM1, PWM2_DAC, PWM3_DAC, PWM4, PWM5, PWM6};  // assing pins
  
  for(uint8_t i = 0; i < NUMBER_OF_DIM_CHAN; i++) {
    channels[i] = new HBWDimmerAdvanced(PWMOut[i], &(hbwconfig.dimCfg[i]));
  };
#else
  #error Dimming channel count and pin missmatch!
#endif

#if NUMBER_OF_INPUT_CHAN == 10 && NUMBER_OF_SEN_INPUT_CHAN == 10

  byte digitalInput[10] = {IO1, IO2, IO3, IO4, ADC1, ADC2, ADC3, ADC4, ADC5, ADC6};  // assing pins
  
  for(uint8_t i = 0; i < NUMBER_OF_INPUT_CHAN; i++) {
    channels[i +NUMBER_OF_DIM_CHAN] = new HBWSenSC(digitalInput[i], &(hbwconfig.senCfg[i]));
    channels[i +NUMBER_OF_DIM_CHAN + NUMBER_OF_INPUT_CHAN] = new HBWKey(digitalInput[i], &(hbwconfig.keyCfg[i]));
  };
#else
  #error Input channel count and pin missmatch!
#endif


#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBDimIODevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN,(HBWChannel**)channels,
                             NULL,
                             new HBWLinkKey(NUM_LINKS_INPUT, LINKADDRESSSTART_INPUT), new HBWLinkDimmerAdvanced(NUM_LINKS_DIM, LINKADDRESSSTART_DIM));
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
#else
  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial
  
  device = new HBDimIODevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
                             new HBWLinkKey(NUM_LINKS_INPUT, LINKADDRESSSTART_INPUT), new HBWLinkDimmerAdvanced(NUM_LINKS_DIM, LINKADDRESSSTART_DIM));
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default

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
};
