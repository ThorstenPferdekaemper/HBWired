//*******************************************************************
//
// HBW-DIS-Key-4
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// LCD Display mit 4 Tastern
// 
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial version
// v0.20
// - add option to configure 1*4 to 4*24 LCD
// - allow to save custom text line to EERPOM (one per display line channel) and load on start
// - added auto-off option for display backlight (dim channel)


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0016
#define HMW_DEVICETYPE 0x71 //device ID (make sure to import hbw-dis-key-4.xml into FHEM)


#define NUMBER_OF_DISPLAY_CHAN 1
#define NUMBER_OF_DISPLAY_DIM_CHAN 1
#define NUMBER_OF_DISPLAY_LINES MAX_DISPLAY_LINES // defined in "HBWDisplayLCD.h", max. lines the display can have (set actual amount in display channel config!)
#define NUMBER_OF_V_TEMP_CHAN 4   // virtual channels to peer with temperature sensor (or set any 2 byte signed value)
#define NUMBER_OF_V_SWITCH_CHAN 4   // virtual channels to peer as switch (display ON/OFF state), or set by FHEM
#define NUMBER_OF_V_CHAN NUMBER_OF_V_TEMP_CHAN + NUMBER_OF_V_SWITCH_CHAN    // total number of virtual channels
#define NUMBER_OF_KEY_CHAN 4
#define NUM_LINKS_KEY 20
#define NUM_LINKS_V_CHAN 32
#define LINKADDRESSSTART_V_CHAN 0x30  // any actor peering!
#define LINKADDRESSSTART_KEY 0x13C  // any sensor peering!
// check DISPLAY_CUSTOM_LINES_EESTART in "HBWDisplayLCD.h" - select a free EEPROM space for this! (or undefine)

//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWKey.h>
#include <HBWLinkKey.h>
//#include <HBWLinkInfoEventActuator.h>
#include "HBWLinkInfoKeyEventActuator.h"  // special version, combining Key and Info event types
#include <HBWDimBacklight.h>
#include "HBWDisplayLCD.h"


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  
  #define BUTTON_1 A0
  #define BUTTON_2 A1
  #define BUTTON_3 A2
  #define BUTTON_4 A3

  //#define no_used.. 3 //4? //8? WS2812B?
  
  #define LCD_RS 12
  #define LCD_EN 11
  #define LCD_D4 10
  #define LCD_D5 9
  #define LCD_D6 7
  #define LCD_D7 6
  
  #define LCD_BACKLIGHT_PWM 5

  #define LDR_PIN A7
  
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
  
  #define LCD_RS 12
  #define LCD_EN 11
  #define LCD_D4 10
  #define LCD_D5 9
  #define LCD_D6 7
  #define LCD_D7 6
  
  #define LCD_BACKLIGHT_PWM 5

  #define LDR_PIN A7
  
//  #define BLOCKED_TWI_SDA A4  // used by I²C - SDA
//  #define BLOCKED_TWI_SCL A5  // used by I²C - SCL

  #include "FreeRam.h"
  #include "HBWSoftwareSerial.h"
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL

#define LED LED_BUILTIN        // Signal-LED


#define NUMBER_OF_CHAN NUMBER_OF_DISPLAY_CHAN + NUMBER_OF_DISPLAY_DIM_CHAN + NUMBER_OF_DISPLAY_LINES + NUMBER_OF_KEY_CHAN + NUMBER_OF_V_TEMP_CHAN + NUMBER_OF_V_SWITCH_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_display DisCfg[NUMBER_OF_DISPLAY_CHAN]; // 0x07 - 0x08 (address step 2)
  hbw_config_dim_backlight DisDimCfg[NUMBER_OF_DISPLAY_CHAN]; // 0x09 - 0x0A (address step 2)
  hbw_config_display_line DisLineCfg[MAX_DISPLAY_LINES]; // 0x0B - 0x0E (address step 1)
  hbw_config_key KeyCfg[NUMBER_OF_KEY_CHAN]; // 0x0F - 0x16 (address step 2)
  hbw_config_displayVChNum DisTCfg[NUMBER_OF_V_TEMP_CHAN];  // 0x17 - 0x1A (address step 1)
  hbw_config_displayVChBool DisBCfg[NUMBER_OF_V_SWITCH_CHAN];  // 0x1B - 0x1F (address step 1)
} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device

HBWDevice* device = NULL;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

void setup()
{
  // create some pointer used across channels
  HBWDisplayVChannel* displayVChannel[NUMBER_OF_V_CHAN];  // pointer to VChannel channels, to access from HBWDisplay class
  HBWDisplayVChannel* displayLines[MAX_DISPLAY_LINES];  // pointer to VChannel channels, to access from HBWDisplay class

  static const byte BUTTON_PIN[] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4};
  
 #if (NUMBER_OF_KEY_CHAN == MAX_DISPLAY_LINES) // this only works in the same loop, if we have same ammount of display lines and key channels! 
  for(uint8_t i = 0; i < MAX_DISPLAY_LINES; i++) {
    channels[i + NUMBER_OF_DISPLAY_CHAN + NUMBER_OF_DISPLAY_DIM_CHAN] = displayLines[i] = new HBWDisplayLine(&(hbwconfig.DisLineCfg[i]));
    channels[i + NUMBER_OF_DISPLAY_CHAN + NUMBER_OF_DISPLAY_DIM_CHAN + NUMBER_OF_DISPLAY_LINES] = new HBWKey(BUTTON_PIN[i], &(hbwconfig.KeyCfg[i]));
  }
 #else
  #error NUMBER_OF_KEY_CHAN channel missmatch!
 #endif
  
 #if (NUMBER_OF_V_TEMP_CHAN == NUMBER_OF_V_SWITCH_CHAN)
  for(uint8_t i = 0; i < NUMBER_OF_V_TEMP_CHAN; i++) {
    channels[i + NUMBER_OF_DISPLAY_CHAN + NUMBER_OF_DISPLAY_DIM_CHAN + NUMBER_OF_DISPLAY_LINES + NUMBER_OF_KEY_CHAN] =\
      displayVChannel[i] = new HBWDisplayVChNum(&(hbwconfig.DisTCfg[i]));
    channels[i + NUMBER_OF_DISPLAY_CHAN + NUMBER_OF_DISPLAY_DIM_CHAN + NUMBER_OF_DISPLAY_LINES + NUMBER_OF_KEY_CHAN + NUMBER_OF_V_TEMP_CHAN] =\
      displayVChannel[NUMBER_OF_V_TEMP_CHAN + i] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[i]));
  }
 #else
  #error Virual input channel missmatch!
 #endif

  channels[1] = new HBWDimBacklight(&(hbwconfig.DisDimCfg[0]), LCD_BACKLIGHT_PWM, LDR_PIN);
  
  // "master" display as last channel, using the pointer to virtual input and display-line channels
  channels[0] = new HBWDisplay(&lcd, (HBWDisplayVChannel**)displayVChannel, &(hbwconfig.DisCfg[0]), (HBWDisplayVChannel**)displayLines);
  

#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             NULL,
                             new HBWLinkKey<NUM_LINKS_KEY, LINKADDRESSSTART_KEY>(),
                             new HBWLinkInfoKeyEventActuator<NUM_LINKS_V_CHAN, LINKADDRESSSTART_V_CHAN>()
                             );
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin();    // RS485 via SoftwareSerial
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
                             //NULL, NULL
                             new HBWLinkKey<NUM_LINKS_KEY, LINKADDRESSSTART_KEY>(),
                             new HBWLinkInfoKeyEventActuator<NUM_LINKS_V_CHAN, LINKADDRESSSTART_V_CHAN>()
                             );
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default

  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F(" #num_chan "));
  //hbwdebug(NUMBER_OF_CHAN);
  hbwdebug(sizeof(channels)/2);

  // calculate EEPORM start addresses. to put in XML
  hbwdebug(F(" #eeStart Chan, Dis:"));
  hbwdebughex(0x07);
  hbwdebug(F(", Dim:"));
  hbwdebughex(0x07 + sizeof(hbwconfig.DisCfg) + sizeof(hbwconfig.DisDimCfg));
  hbwdebug(F(", Key:"));
  hbwdebughex(0x07 + sizeof(hbwconfig.DisCfg) + sizeof(hbwconfig.DisDimCfg) + sizeof(hbwconfig.DisLineCfg));
  hbwdebug(F(", Vn:"));
  hbwdebughex(0x0B + sizeof(hbwconfig.DisCfg) + sizeof(hbwconfig.DisDimCfg) + sizeof(hbwconfig.DisLineCfg) + sizeof(hbwconfig.KeyCfg));
  hbwdebug(F(", Vb:"));
  hbwdebughex(0x07 + sizeof(hbwconfig.DisCfg) + sizeof(hbwconfig.DisDimCfg) + sizeof(hbwconfig.DisLineCfg) + sizeof(hbwconfig.KeyCfg) + sizeof(hbwconfig.DisTCfg));
  hbwdebug(F("\n"));
#endif
}


void loop()
{
  device->loop();
  POWERSAVE();  // go sleep a bit
};



// check if HBWLinkInfoEvent support is enabled, when links are set
#if !defined(Support_HBWLink_InfoEvent) && defined(NUMBER_OF_V_TEMP_CHAN)
#error enable/define Support_HBWLink_InfoEvent in HBWired.h
#endif
  
