//*******************************************************************
//
// HBW-Sen-DB-4
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// Türklingelsensor mit 4 Tastern und Beleuchtung des Klingeltableau
// 
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.10
// - initial version
// v0.20
// - fix for repeatCounter reset handling
// v0.3
// - added buzzer for button feedback (uses tone() builtin function) - needs new XML due to additional config


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x001E
#define HMW_DEVICETYPE 0x98 //device ID (make sure to import hbw-dis-key-4.xml into FHEM)

// + 1 türsummer?
#define NUMBER_OF_DIM_CHAN 1   // dimmer output (for backlight) - with auto_brightness
#define NUMBER_OF_KEY_CHAN 4   // knobs at your door
#define NUM_LINKS_KEY 20
#define LINKADDRESSSTART_V_CHAN 0x30  // any actor peering!
#define LINKADDRESSSTART_KEY 0x13C  // any sensor peering!

//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkKey.h>
#include "HBWKeyDoorbell.h"
#include <HBWDimBacklight.h>


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  
  #define BUTTON_1 8
  #define BUTTON_2 7
  #define BUTTON_3 4
  #define BUTTON_4 A1

  #define BUZZER 11 // buzzer for key press feedback
  #define BACKLIGHT_PWM 9  // timer1 (pin 9 & 10)
  #define LDR_PIN A0
  
//  #define BLOCKED_TWI_SDA A4  // used by I²C - SDA
//  #define BLOCKED_TWI_SCL A5  // used by I²C - SCL
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.

  #define BUTTON_1 A0
  #define BUTTON_2 A1
  #define BUTTON_3 A2
  #define BUTTON_4 A3
  
  #define BUZZER 11 // buzzer for key press feedback  (tone() timer prio: timer2, 1, 0)
  // 11, 3 (controlled by timer2), timer0 (pin 5 & 6), timer1 (pin 9 & 10)
  #define BACKLIGHT_PWM 9
  #define LDR_PIN A7
  
//  #define BLOCKED_TWI_SDA A4  // used by I²C - SDA
//  #define BLOCKED_TWI_SCL A5  // used by I²C - SCL

  #include "FreeRam.h"
  #include "HBWSoftwareSerial.h"
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL

#define LED LED_BUILTIN        // Signal-LED


#define NUMBER_OF_CHAN NUMBER_OF_DIM_CHAN + NUMBER_OF_KEY_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_dim_backlight BlDimCfg[NUMBER_OF_DIM_CHAN]; // 0x07 - 0x08 (address step 2)
  hbw_config_key_doorbell DBKeyCfg[NUMBER_OF_KEY_CHAN]; // 0x09 - 0x1C (address step 5)
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device

HBWDevice* device = NULL;


void setup()
{
  channels[0] = new HBWDimBacklight(&(hbwconfig.BlDimCfg[0]), BACKLIGHT_PWM, LDR_PIN);

  
  static const byte BUTTON_PIN[] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4};
  
 #if (NUMBER_OF_KEY_CHAN == 4)
  for(uint8_t i = 0; i < NUMBER_OF_KEY_CHAN; i++) {
    channels[i + NUMBER_OF_DIM_CHAN] = new HBWKeyDoorbell(BUTTON_PIN[i], &(hbwconfig.DBKeyCfg[i]), BUZZER);
  }
 #else
  #error NUMBER_OF_KEY_CHAN channel missmatch!
 #endif
  
 
#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             NULL,
                             new HBWLinkKey<NUM_LINKS_KEY, LINKADDRESSSTART_KEY>(),
                             NULL
                             );
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin();    // RS485 via SoftwareSerial
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
                             new HBWLinkKey<NUM_LINKS_KEY, LINKADDRESSSTART_KEY>(),
                             NULL
                             );
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default

  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F(" #num_chan "));
  //hbwdebug(NUMBER_OF_CHAN);
  hbwdebug(sizeof(channels)/2);

  // calculate EEPORM start addresses. to put in XML
  hbwdebug(F(" #eeStart Chan, Dim:"));
  hbwdebughex(0x07);
  hbwdebug(F(", Key:"));
  hbwdebughex(0x07 + sizeof(hbwconfig.BlDimCfg));
  hbwdebug(F("\n"));
#endif
}


void loop()
{
  device->loop();
  POWERSAVE();  // go sleep a bit
};
