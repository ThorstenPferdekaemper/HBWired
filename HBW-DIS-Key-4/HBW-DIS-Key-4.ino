//*******************************************************************
//
// HBW-DIS-Key-4
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.01
// - initial version



#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0001
#define HMW_DEVICETYPE 0x71 //device ID (make sure to import hbw-dis-key-4.xml into FHEM)


//currently defined in "HBWDisplayLCD.h"#define NUMBER_OF_DISPLAY_LINES 2
#define NUMBER_OF_DISPLAY_CHAN 1
#define NUMBER_OF_DISPLAY_DIM_CHAN 1
#define NUMBER_OF_V_TEMP_CHAN 4   // virtual channels to peer with temperature sensor (or set any 2 byte signed value)
#define NUMBER_OF_V_SWITCH_CHAN 4   // virtual channels to peer as switch (display ON/OFF state), or set by FHEM
//currently defined in "HBWDisplayLCD.h" #define NUMBER_OF_V_CHAN NUMBER_OF_V_TEMP_CHAN + NUMBER_OF_V_SWITCH_CHAN    // total number of virtual channels
#define NUMBER_OF_KEY_CHAN 4
#define NUM_LINKS_KEY 20
#define NUM_LINKS_V_CHAN 32
#define LINKADDRESSSTART_V_CHAN 0x30  // any actor peering!
#define LINKADDRESSSTART_KEY 0x13C  // any sensor peering!

//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWKey.h>
#include <HBWLinkKey.h>
//#include <HBWLinkInfoEventActuator.h>
#include "HBWLinkInfoKeyEventActuator.h"  // special version, combining Key and Info event types
#include "HBWDisplayLCD.h"


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  
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
  
  #define LCD_BACKLIGHT 5

  #define LDR A7
  
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
  
  #define LCD_BACKLIGHT 5

  #define LDR A7
  
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
  hbw_config_display DisCfg[NUMBER_OF_DISPLAY_CHAN]; // 0x07 - 0x (address step 2)
  hbw_config_display_backlight DisDimCfg[NUMBER_OF_DISPLAY_CHAN]; // 0x09 - 0x (address step 2)
  hbw_config_display_line DisLineCfg[NUMBER_OF_DISPLAY_LINES]; // 0x0B - 0x (address step 1)
  hbw_config_key KeyCfg[NUMBER_OF_KEY_CHAN]; // 0x0D - 0x (address step 2)
  hbw_config_displayVChNum DisTCfg[NUMBER_OF_V_TEMP_CHAN];  // 0x15 - 0x (address step 1)
  hbw_config_displayVChBool DisBCfg[NUMBER_OF_V_SWITCH_CHAN];  // 0x19 - 0x (address step 1)
} hbwconfig;

//typedef struct {
//  uint8_t blocked;  // don't use 0x0
//  uint8_t logging_time;     // 0x01
//  uint32_t central_address;  // 0x02 - 0x05
//  uint8_t direct_link_deactivate:1;   // 0x06:0
//  uint8_t              :7;   // 0x06:1-7
//  hbw_config_display DisCfg[NUMBER_OF_DISPLAY_CHAN]; // 0x07 - 0x (address step 2)
//  hbw_config_display_backlight DisDimCfg[NUMBER_OF_DISPLAY_CHAN]; // 0x09 - 0x (address step 2)
//  hbw_config_display_line DisLineCfg[NUMBER_OF_DISPLAY_LINES]; // 0x0B - 0x (address step 1)
//  hbw_config_key KeyCfg[NUMBER_OF_KEY_CHAN]; // 0x0D - 0x (address step 2)
//  hbw_config_displayVChNum DisTCfg[NUMBER_OF_V_TEMP_CHAN];  // 0x15 - 0x (address step 1)
//  hbw_config_displayVChBool DisBCfg[NUMBER_OF_V_SWITCH_CHAN];  // 0x19 - 0x (address step 1)
//  t_hbw_config_display config_Display;
//} t_hbw_config;
//
//t_hbw_config hbw_config EEMEM;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device

HBWDevice* device = NULL;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

void setup()
{
  // create some variables used across different channels
  //t_displayVChVars displayVChannelVars[2];
  HBWDisplayVChannel* displayVChannel[NUMBER_OF_V_CHAN];  // pointer to VChannel channels, to access from HBWDisplay class
  HBWDisplayVChannel* displayLines[NUMBER_OF_DISPLAY_LINES];  // pointer to VChannel channels, to access from HBWDisplay class

  //t_displayVChanValues dispVChanVars[4] = {NULL};
  //t_displayLine displayLines[2];


  displayLines[0] = new HBWDisplayLine(&(hbwconfig.DisLineCfg[0]));//, (char*)&displayLines[0].line);
  displayLines[1] = new HBWDisplayLine(&(hbwconfig.DisLineCfg[1]));//, (char*)&displayLines[1].line);
  channels[2] = displayLines[0];
  channels[3] = displayLines[1];  
  
  channels[4] = new HBWKey(BUTTON_1, &(hbwconfig.KeyCfg[0]));
  channels[5] = new HBWKey(BUTTON_2, &(hbwconfig.KeyCfg[1]));
  channels[6] = new HBWKey(BUTTON_3, &(hbwconfig.KeyCfg[2]));
  channels[7] = new HBWKey(BUTTON_4, &(hbwconfig.KeyCfg[3]));
  
  displayVChannel[0] = new HBWDisplayVChNum(&(hbwconfig.DisTCfg[0]));//, &displayVChannelVars[0].n);
  channels[8] = displayVChannel[0];
  displayVChannel[1] = new HBWDisplayVChNum(&(hbwconfig.DisTCfg[1]));//, &displayVChannelVars[1].n);
  channels[9] = displayVChannel[1];
  channels[10] = displayVChannel[2] = new HBWDisplayVChNum(&(hbwconfig.DisTCfg[2]));
  channels[11] = displayVChannel[3] = new HBWDisplayVChNum(&(hbwconfig.DisTCfg[3]));
  
  //channels[3] = new HBWDisplayVChNum(&(hbwconfig.DisTCfg[1]), &displayVChannelVars.t[1]);
/////////  channels[4] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[0]), dispVChanVars);
//  channels[4] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[0]), (char*)&displayVChannelVars[0].b);
//  channels[5] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[1]), (char*)&displayVChannelVars[1].b);
//  displayVChannel[4] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[0]));//, &dispVChanVars[0].getStrFromVal);
//  displayVChannel[5] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[1]));//, &dispVChanVars[1].getStrFromVal);
  channels[12] = displayVChannel[4] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[0]));
  channels[13] = displayVChannel[5] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[1]));
  channels[14] = displayVChannel[6] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[2]));
  channels[15] = displayVChannel[7] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[3]));

  channels[0] = new HBWDisplay(&lcd, (HBWDisplayVChannel**)displayVChannel, &(hbwconfig.DisCfg[0]), (HBWDisplayVChannel**)displayLines);
  channels[1] = new HBWDisplayDim(&(hbwconfig.DisDimCfg[0]), LCD_BACKLIGHT, LDR);
  
  //channels[0] = new HBWDisplay(&lcd, dispVChanVars, &(hbwconfig.DisCfg[0]));
//  channels[5] = new HBWDisplayVChBool(&(hbwconfig.DisBCfg[1]), &displayVChannelVars.b[1]);


  
//  byte buttonPin[NUMBER_OF_KEY_CHAN] = {BUTTON_1, BUTTON_2, BUTTON_3};  // assing pins
//  for(uint8_t i = 0; i < NUMBER_OF_KEY_CHAN; i++) {
//    channels[i +NUMBER_OF_TEMP_CHAN +NUMBER_OF_DELTAT_CHAN *3] = new HBWKey(buttonPin[i], &(hbwconfig.KeyCfg[i]));
//  }


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
  hbwdebug(F("\n"));
#endif
}


void loop()
{
  device->loop();
};



// check if HBWLinkInfoEvent support is enabled, when links are set
#if !defined(Support_HBWLink_InfoEvent) && defined(NUMBER_OF_V_TEMP_CHAN)
#error enable/define Support_HBWLink_InfoEvent in HBWired.h
#endif
  
