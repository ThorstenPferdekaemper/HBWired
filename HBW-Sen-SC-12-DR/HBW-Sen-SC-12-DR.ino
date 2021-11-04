//*******************************************************************
//
// hbw_sen_sc_12_dr
//
// DANKSAGUNG:
// Diese Arbeit basiert sich auf HBW-SC-10-Dim-6 und wurde mit Hilfe von loetmeister und 
// Tutorial und Hilfe von Thorsten (Mamber FHEM Forum "Thorsten Pferdekaemper") fertiggestellt.
// Hiermit bedanke ich mich bei Thorsten und loetmeister für die hervorragende Arbeit mit HomeBrewWired und das Tutorial.
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 12 digitale Eingänge (Key/Taster & Sensor) & Sensorkontakte
// Diese Arbeit basiert sich auf HBW-SC-10-Dim-6 und wurde mit Hilfe von loetmeister und 
// Tutorial und Hilfe von Thorsten (Mamber FHEM Forum "Thorsten Pferdekaemper")
//
// Juri - JARA Armenia


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x003C
#define HMW_DEVICETYPE 0xA6  // device ID (make sure to import hbw_sen_sc_12_dr.xml into FHEM)

#define NUMBER_OF_INPUT_CHAN 12   // input channel - pushbutton, key, other digital in
#define NUMBER_OF_SEN_INPUT_CHAN 12  // equal number of sensor channels, using same ports/IOs as INPUT_CHAN


#define NUM_LINKS_INPUT 20    // address step 6
#define LINKADDRESSSTART_INPUT 0x038   // ends @0x0C7


//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


// HB Wired protocol and module
#include <HBWired.h>
#include <HBWLinkKey.h>
#include <HBWKey.h>
#include <HBWSenSC.h>


// Pins
  #define IO1 A3
  #define IO2 10
  #define IO3 11
  #define IO4 A0
  #define IO5 A1
  #define IO6 A2
  #define IO7 A4
  #define IO8 A5
  #define IO9 9  // dummy pin to fill the array elements
  #define IO10 7  // dummy pin to fill the array elements
  #define IO11 6
  #define IO12 5

#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  
  #include "FreeRam.h"
  #include <HBWSoftwareSerial.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif

#define LED LED_BUILTIN        // Signal-LED

#define NUMBER_OF_CHAN NUMBER_OF_INPUT_CHAN + NUMBER_OF_SEN_INPUT_CHAN


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7

  hbw_config_senSC senCfg[NUMBER_OF_SEN_INPUT_CHAN]; // 0x07 - 0x12 (address step 1)
  hbw_config_key keyCfg[NUMBER_OF_INPUT_CHAN]; // 0x13 - 0x2A (address step 2)

} hbwconfig;


HBWChannel* channels[NUMBER_OF_CHAN];  // total number of channels for the device

HBWDevice* device = NULL;


void setup()
{
  // create channels

#if NUMBER_OF_INPUT_CHAN == 12 && NUMBER_OF_SEN_INPUT_CHAN == 12
  static const uint8_t digitalInput[12] = {IO1, IO2, IO3, IO4, IO5, IO6, IO7, IO8, IO9, IO10, IO11, IO12};  // assing pins

  // input sensor and key channels
  for(uint8_t i = 0; i < NUMBER_OF_SEN_INPUT_CHAN; i++) {
    channels[i] = new HBWSenSC(digitalInput[i], &(hbwconfig.senCfg[i]));
    channels[i + NUMBER_OF_SEN_INPUT_CHAN] = new HBWKey(digitalInput[i], &(hbwconfig.keyCfg[i]));
  };
#else
  #error Input channel count and pin missmatch!
#endif


#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             NULL,
                             new HBWLinkKey<NUM_LINKS_INPUT, LINKADDRESSSTART_INPUT>(), NULL);
  
  device->setConfigPins(BUTTON, LED);
  
#else
  Serial.begin(115200);  // Serial->USB for debug
  rs485.begin(19200);   // RS485 via SoftwareSerial, must use 19200 baud!
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                             &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                             NUMBER_OF_CHAN, (HBWChannel**)channels,
                             &Serial,
                             new HBWLinkKey<NUM_LINKS_INPUT, LINKADDRESSSTART_INPUT>(), NULL);
  
  device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default
  //device->setStatusLEDPins(LED, LED); // Tx, Rx LEDs

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
