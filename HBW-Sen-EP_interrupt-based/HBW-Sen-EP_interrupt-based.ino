//*******************************************************************
//
// HBW-Sen-EP, RS485 8-channel Puls Counter / S0 interface
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 
// HBW-Sen-EP zum Zählen von elektrischen Pulsen (z.B. S0-Schnittstelle)
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial version
// v0.02
// - rework of channel loop - reading input
// - removed POLLING_TIME config (replaced by fixed debounce time - DEBOUNCE_DELAY set in HBWSenEP.h)
// - add interrupt and LCD support (-> separate project file HBW-Sen-EP_interrupt.ino)
// v0.2
// - made compatible for ATMEGA 328PB (https://github.com/watterott/Arduino-Boards / https://github.com/MCUdude/MiniCore)
// v0.3
// - moved from FALLING to CHANGE pin change interrupt for better debounce handling


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x001E
#define HMW_DEVICETYPE 0x84    //device ID (make sure to import hbw-sen-ep.xml into FHEM)

#define NUMBER_OF_SEN_CHAN 8   // input channels


// HB Wired protocol and modules
#include <HBWired.h>
#include <HBWSenEP.h>
#include <HBW_eeprom.h>


// Pins and hardware config
#include "HBW-Sen-EP_interrupt-based_config_example.h"  // When using custom device pinout or controller, copy this file and include it instead
// #include "HBW-Sen-EP_interrupt-based_config_ownLCD.h"


#define NUMBER_OF_CHAN NUMBER_OF_SEN_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_sen_ep SenEpCfg[NUMBER_OF_SEN_CHAN]; // 0x07 - 0x..(address step 9)
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device
HBWDevice* device = NULL;


#if defined(USE_HARDWARE_SERIAL) && defined(USE_INTERRUPTS_FOR_INPUT_PINS)
#if NUMBER_OF_SEN_CHAN == 8
// create interrupt functions
interruptFunction(Sen1);
interruptFunction(Sen2);
interruptFunction(Sen3);
interruptFunction(Sen4);
interruptFunction(Sen5);
interruptFunction(Sen6);
interruptFunction(Sen7);
interruptFunction(Sen8);
#else
  #error Input channel missmatch!
#endif
#endif

void setup()
{
  // create channels
 #if !defined(USE_INTERRUPTS_FOR_INPUT_PINS)
  static const uint8_t SenPin[NUMBER_OF_SEN_CHAN] = {Sen1, Sen2, Sen3, Sen4, Sen5, Sen6, Sen7, Sen8};  // assing pins
  
  for(uint8_t i = 0; i < NUMBER_OF_SEN_CHAN; i++) {
    channels[i] = new HBWSenEP(SenPin[i], &(hbwconfig.SenEpCfg[i]));
  }
  
 #elif defined(USE_HARDWARE_SERIAL) && defined(USE_INTERRUPTS_FOR_INPUT_PINS)
  #if NUMBER_OF_SEN_CHAN == 8
  //TODO: find smarter way to point to ISR counter variable? e.g. use struct? count += sizeof(uint16_t PINCOUNT(x)??)
  channels[0] = new HBWSenEP(&getInterruptCounter(Sen1), &(hbwconfig.SenEpCfg[0]));
  channels[1] = new HBWSenEP(&getInterruptCounter(Sen2), &(hbwconfig.SenEpCfg[1]));
  channels[2] = new HBWSenEP(&getInterruptCounter(Sen3), &(hbwconfig.SenEpCfg[2]));
  channels[3] = new HBWSenEP(&getInterruptCounter(Sen4), &(hbwconfig.SenEpCfg[3]));
  channels[4] = new HBWSenEP(&getInterruptCounter(Sen5), &(hbwconfig.SenEpCfg[4]));
  channels[5] = new HBWSenEP(&getInterruptCounter(Sen6), &(hbwconfig.SenEpCfg[5]));
  channels[6] = new HBWSenEP(&getInterruptCounter(Sen7), &(hbwconfig.SenEpCfg[6]));
  channels[7] = new HBWSenEP(&getInterruptCounter(Sen8), &(hbwconfig.SenEpCfg[7]));
  #else
    #error Input channel missmatch!
  #endif
 #endif

#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUMBER_OF_CHAN, (HBWChannel**)channels,
                         NULL,
                         NULL, NULL);
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin(19200);   // RS485 via SoftwareSerial, must use 19200 baud!
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUMBER_OF_CHAN, (HBWChannel**)channels,
                         &Serial,
                         NULL, NULL);
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default

  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
#endif

#ifdef USE_LCD
  LCdisplay.begin();
  pinMode(LCD_BUTTON, INPUT_PULLUP);
#endif
#if defined(USE_HARDWARE_SERIAL) && defined(USE_INTERRUPTS_FOR_INPUT_PINS)
  #if NUMBER_OF_SEN_CHAN == 8
  // enable interrupts finally
  setupInterrupt(Sen1);
  setupInterrupt(Sen2);
  setupInterrupt(Sen3);
  setupInterrupt(Sen4);
  setupInterrupt(Sen5);
  setupInterrupt(Sen6);
  setupInterrupt(Sen7);
  setupInterrupt(Sen8);
  #else
   #error Input channel missmatch!
  #endif
#endif
}


void loop()
{
  device->loop();

//   //LCdisplay.bell.toggle();
 #if defined(USE_LCD)
  static uint32_t lastUpdate = 0, LCDbuttonLastPress = 0;
  static uint8_t ch_count = 0;
  uint32_t now = millis();
  static const uint16_t UPDATE_AND_DEBOUNCE_TIME = 200;
  if (now - lastUpdate > UPDATE_AND_DEBOUNCE_TIME) {
    LCdisplay.eraseNumbers();
    LCdisplay.put(('0' + (ch_count +1) % 10), 0);
    LCdisplay.put(':', 1);
   #if defined(USE_INTERRUPTS_FOR_INPUT_PINS)
   //TODO: find smarter way to point to ISR counter variable? e.g. use struct?
// static const senPins[] = {Sen1, Sen2, ...}; LCdisplay.put(getInterruptCounter(senPins[ch_count]));
    switch (ch_count) {
      case 0:
        LCdisplay.put(getInterruptCounter(Sen1));
      break;
      case 1:
        LCdisplay.put(getInterruptCounter(Sen2));
      break;
      case 2:
        LCdisplay.put(getInterruptCounter(Sen3));
      break;
      case 3:
        LCdisplay.put(getInterruptCounter(Sen4));
      break;
      case 4:
        LCdisplay.put(getInterruptCounter(Sen5));
      break;
      case 5:
        LCdisplay.put(getInterruptCounter(Sen6));
      break;
      case 6:
        LCdisplay.put(getInterruptCounter(Sen7));
      break;
      case 7:
        LCdisplay.put(getInterruptCounter(Sen8));
      break;
    }
   #else
    uint8_t level_be[2];  // level comes in "Big Endian"
    device->get(ch_count, level_be);
    uint16_t level = level_be[1] | (level_be[0] << 8);
    LCdisplay.put(level);
   #endif
    LCdisplay.flush();

    if (digitalRead(LCD_BUTTON) == LOW) {
      if (!LCDbuttonLastPress) {
        LCDbuttonLastPress = (now == 0) ? 1 : now;
      }
      else if ((int32_t)(now - LCDbuttonLastPress) > UPDATE_AND_DEBOUNCE_TIME) {
        LCDbuttonLastPress = now + UPDATE_AND_DEBOUNCE_TIME*4;  // when remains pressed, add some delay
        if (ch_count < 7) ch_count++; else ch_count = 0;
      }
    }
    else {
      LCDbuttonLastPress = 0;
    }
    lastUpdate = now;
  }
 #endif // USE_LCD

  POWERSAVE();  // go sleep a bit
};
