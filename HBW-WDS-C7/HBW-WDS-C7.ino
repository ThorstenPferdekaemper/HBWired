//*******************************************************************
//
// HBW-WDS-C7
//
// Homematic Wired Hombrew Hardware
// Raspberry Pi Pico als Homematic-Device
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
// Arduino Boards: https://github.com/earlephilhower/arduino-pico
//
// Diese Modul stellt die Messwerte einer Bresser 7 in 1 oder 6 in 1
// Wetterstation als Homematic Wired Gerät zur Verfügung.
// Die Basis ist ein SIGNALDuino mit cc1101 868MhZ Modul:
// https://github.com/Ralf9/SIGNALDuino/tree/dev-r335_cc1101
// Es kann parallel am RS485 Bus und USB betrieben werden. Die Funktion
// des SIGNALDuino (advanced) ist nicht eingeschränkt.
//
//*******************************************************************
// Changes
// v0.01
// - initial version/testing


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0001
#define HMW_DEVICETYPE 0x88

#define NUM_CHANNELS 2
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x80


/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove code not needed. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */

#define Support_ModuleReset

#include <FreeRam.h>
#include <HBW_eeprom.h>
// HB Wired protocol and module
#include <HBWired.h>
#include "HBWSIGNALDuino_adv.h"
#include "HBWSIGNALDuino_bresser7in1.h"


/* harware specifics ------------------------ */
#if defined (ARDUINO_ARCH_RP2040)
  // #include "RPi_Pico_TimerInterrupt.h"  // https://github.com/khoih-prog/RPI_PICO_TimerInterrupt
  // RPI_PICO_Timer Timer1(1);
  #include <Wire.h>
  AT24C128* EepromPtr = new AT24C128(AT24C_ADDRESS_0);
  // Eeprom24C04_16* EepromPtr = new Eeprom24C04_16(AT24C_ADDRESS_0);
#else
  #error Target Plattform not supported! Please contribute.
#endif

// Pins
// TODO move to own file (pins_default.h pins_custom.h - don't check-in last one)
#define LED 3//LED_BUILTIN      // Signal-LED

#define RS485_TXEN 2  // Transmit-Enable
#define BUTTON 22  // Button fuer Factory-Reset etc.

// cc1101 @ SPI0
// #define PIN_SEND              20   // gdo0Pin TX out
// #define PIN_RECEIVE           21   // gdo2

// default pins:
// USB Rx / Tx 0, 1
// static const uint8_t SPIpins[] = {MISO, MOSI, SS, SCK}; // pin 16 - 19
// static const uint8_t I2Cpins[] = {PIN_WIRE_SDA, PIN_WIRE_SCL}; // pin 4 & 5
// UART1 8 & 9

static struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_signalduino_adv signalduinoCfg[NUM_CHANNELS]; // 0x07-0x... ? (address step ?)
  hbw_config_signalduino_wds_7in1 wds7in1Cfg[NUM_CHANNELS]; // 0x07-0x... ? (address step ?)
  // hbw_config_signalduino_wds wds7in1Cfg[NUM_CHANNELS]; // 0x07-0x... ? (address step ?)
  // TempH channel? (provide temp and humidity in separate channel for peering?)
} hbwconfig;


// create object for the channel
HBWChannel* channels[NUM_CHANNELS];


class HBWDSDevice : public HBWDevice {
    public: 
    HBWDSDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
               Stream* _rs485, uint8_t _txen,
               uint8_t _configSize, void* _config, 
               uint8_t _numChannels, HBWChannel** _channels,
               Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
    HBWDevice(_devicetype, _hardware_version, _firmware_version,
              _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
              _debugstream, linksender, linkreceiver) {
    };
    // virtual void afterReadConfig();
};

// device specific defaults
// void HBWDSDevice::afterReadConfig() {
//   if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
// };


HBWDSDevice* device = NULL;


/*--------------------------------------------SIGNALDuino-------------------------------------------------*/
#include "HBWSIGNALDuino_adv/SIGNALDuino.ino.hpp"
/* providing setup() and loop() for core0 */
/*--------------------------------------------SIGNALDuino-------------------------------------------------*/

// #include "EEPROM_24cXX.h"
// core1 running the Homematic device
void setup1()
{
  delay(3000);
  // pinMode(LED, OUTPUT);
  // digitalWrite(LED, HIGH);
  Serial.begin(115200);  // Serial->USB for debug (shared with SIGNALDuino, turn debug off?)
  Serial1.begin(19200, SERIAL_8E1);  // RS485 bus
delay(1000);Serial.println("init1");

// EE dump
// uint8_t ee_buff[16];
// for (uint i = 0; i < 2064; i+=16) {
// // int totalRead = EepromPtr->readBuffer(i, ee_buff, sizeof(ee_buff));
// Serial.print("EE ");Serial.print(i);Serial.print(": ");
// for (uint x = 0; x < 16; x++){
// // Serial.print(ee_buff[x], HEX);Serial.print(" ");
// Serial.print(EepromPtr->read(x+i), HEX);Serial.print(" ");
// }
// // Serial.print(":");Serial.print(totalRead);
// Serial.println("");
// }
  #if defined (ARDUINO_ARCH_RP2040)
    // Wire.begin();
    // EepromPtr->setMemoryType(EEPROM_Memory_Type);
    // if (! EepromPtr->begin())
    // {
    //   Serial.println("No EEPROM! Halting!");  // stop without EEPROM
    //   // TODO: disable watchdog to stay in this loop
    //   while (true) {
    //     digitalWrite(LED, HIGH);
    //     delay(200);
    //     digitalWrite(LED, LOW);
    //     delay(200);
    //   }
    // }
	#endif

  // create channels
  // static const uint8_t pins[NUM_CHANNELS] = {SWITCH1_PIN, SWITCH2_PIN};
  
  // creating to channels
  // for(uint8_t i = 0; i < NUM_CHANNELS; i++) {
    channels[0] = new HBWSIGNALDuino_adv(PIN_RECEIVE, PIN_SEND, PIN_LED, &(hbwconfig.signalduinoCfg[0]));
    // channels[1] = new HBWSIGNALDuino_bresser7in1(ccBuf, &g_ccBuf_ready, &(hbwconfig.wds7in1Cfg[0]));
    channels[1] = new HBWSIGNALDuino_bresser7in1(ccBuf, g_hbw_link, &(hbwconfig.wds7in1Cfg[0]));
  // }
// delay(500);Serial.println("create dev");
  // create the device
  device = new HBWDSDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &Serial1,
                         RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                         NUM_CHANNELS, (HBWChannel**)channels,
                        //  NULL, new HBWLinkSwitchAdvanced<NUM_LINKS, LINKADDRESSSTART>());
                         &Serial);

  device->setConfigPins(BUTTON, LED);
  
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n"));

  Serial.println("setup1 done");
  // digitalWrite(LED, LOW);
};


// setup for second core
// void setup1() {
//   delay(1000);  Serial.println("core2");

// };

// void loop()
// {
  // device->loop();
  // POWERSAVE();  // go sleep a bit
// };


// loop for second core
void loop1() {
  device->loop();

// can only read once from serial buffer...
  // if (Serial.available()) {
  //   char c = Serial.read();
  //   if (c == 'g') {
  //     uint8_t data[2];
  //     device->get(0, data);
  //     Serial.print("get: ");Serial.println(data[0]);
  //   }
  //   if (c == 'c') {
  //     Serial.print("dev conf, centralAddr: ");Serial.print(hbwconfig.central_address);
  //     Serial.print(" ownAddr: ");Serial.println(device->getOwnAddress());
  //         //  uint8_t aData[4] = {0x42, 0x00, 0x09, 0x99}; //1107298713 | 99090042 | 2576351268
  //         //  for(byte i = 0; i < 4; i++) {
  //       	//    EepromPtr->write(E2END - 3 + i, aData[i]);
  //         //  }
  //   }
    // if (c == 'm') {
    // if (!(millis() % 15000)) {
    //     EepromPtr->read(0x0); // dummy read, to check result
    //     uint8_t ee_err = EepromPtr->getLastError();
    //     if (ee_err == 0)
    //     {
    //       uint32_t memSize = EepromPtr->length();
    //       Serial.print("EEProm Size bytes ");
    //       Serial.println(memSize);
    //       Serial.print("read magic byte ");
    //       Serial.println(EepromPtr->read(0x0));
    //     }
    //     else {
    //       Serial.print(ee_err);
    //       Serial.println(" No Memory detected!");
    //     }
    // }
  // }
};
