//*******************************************************************
//
// HBW-LC-Sw-8
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// - Active HIGH oder LOW kann konfiguriert werden
//
//*******************************************************************


#define HMW_DEVICETYPE 0x83
#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0100

#define NUM_CHANNELS 8
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x40

#include "HBWSoftwareSerial.h"
#include "FreeRam.h"    


// HB Wired protocol and module
#include "HBWired.h"
#include "HBWLinkSwitchSimple.h"
#include "HBWSwitch.h"

#define RS485_RXD 4
#define RS485_TXD 2
#define RS485_TXEN 3  // Transmit-Enable

// HBWSoftwareSerial can only do 19200 baud
HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX


// Pins
#define BUTTON 8  // Button fuer Factory-Reset etc.
#define LED LED_BUILTIN        // Signal-LED

// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
#define PIN_ARRAY uint8_t pins[NUM_CHANNELS] = {A0, A1, A2, A3, A4, A5, 10, 11};


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_switch switchcfg[NUM_CHANNELS]; // 0x07-0x... ?
} hbwconfig;




HBWChannel* switches[NUM_CHANNELS];
HBWDevice* device = NULL;



void setup()
{
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial

   // create channels
   PIN_ARRAY
  // assing switches (relay) pins
   for(uint8_t i = 0; i < NUM_CHANNELS; i++){
      switches[i] = new HBWSwitch(pins[i], &(hbwconfig.switchcfg[i]));
   };

  device = new HBWDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485,RS485_TXEN,sizeof(hbwconfig),&hbwconfig,
                         NUM_CHANNELS,(HBWChannel**)switches,
                         &Serial,
                         NULL, new HBWLinkSwitchSimple(NUM_LINKS,LINKADDRESSSTART));

  device->setConfigPins();  // 8 and 13 is the default
 
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));
}


void loop()
{
  device->loop();
};

