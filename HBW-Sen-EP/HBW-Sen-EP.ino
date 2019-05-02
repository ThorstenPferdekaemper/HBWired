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


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0001

#define NUMBER_OF_SEN_CHAN 8   // input channels

#define HMW_DEVICETYPE 0x84 //device ID (make sure to import hbw_ .xml into FHEM)


//#define USE_HARDWARE_SERIAL   // use hardware serial (USART) - this disables debug output


// HB Wired protocol and modules
#include <HBWired.h>
#include "HBWSenEP.h"


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage
  
  #define Sen1 A0    // digital input
  #define Sen2 A1
  #define Sen3 A2
  #define Sen4 A3
  #define Sen5 A4
  #define Sen6 A5
  #define Sen7 3
  #define Sen8 7

#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage

  #define Sen1 A0    // digital input
  #define Sen2 A1
  #define Sen3 A2
  #define Sen4 A3
  #define Sen5 A4
  #define Sen6 A5
  #define Sen7 5
  #define Sen8 7
  
  #include "FreeRam.h"
  #include "HBWSoftwareSerial.h"
  // HBWSoftwareSerial can only do 19200 baud
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL

#define LED LED_BUILTIN        // Signal-LED

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


void setup()
{
  // create channels
  byte SenPin[NUMBER_OF_SEN_CHAN] = {Sen1, Sen2, Sen3, Sen4, Sen5, Sen6, Sen7, Sen8};  // assing pins
  
  for(uint8_t i = 0; i < NUMBER_OF_SEN_CHAN; i++) {
    channels[i] = new HBWSenEP(SenPin[i], &(hbwconfig.SenEpCfg[i]));
  }


#ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
  Serial.begin(19200, SERIAL_8E1);
  
  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUMBER_OF_CHAN, (HBWChannel**)channels,
                         NULL,
                         NULL, NULL);
  
  device->setConfigPins(BUTTON, LED);  // use analog input for 'BUTTON'
  
#else
  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial
  
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
}


void loop()
{
  device->loop();
};
