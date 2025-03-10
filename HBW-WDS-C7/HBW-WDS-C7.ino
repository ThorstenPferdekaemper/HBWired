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
// Diese Modul stellt die Messwerte einer Bresser 7 in 1 [oder 5 in 1]
// Wetterstation als Homematic Wired Gerät zur Verfügung.
// Die Basis ist ein SIGNALDuino mit cc1101 868MhZ Modul:
// https://github.com/Ralf9/SIGNALDuino/tree/dev-r335_cc1101
// Es kann parallel am RS485 Bus und USB betrieben werden. Die Funktion
// des SIGNALDuino (advanced) ist nicht eingeschränkt.
//
//*******************************************************************
// Changes
// v0.03
// - initial version/testing
// v0.1
// - added timeout handling for HBWSIGNALDuino_bresser7in1
// - fixed storm status (HBWSIGNALDuino_bresser7in1)


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x000D
#define HMW_DEVICETYPE 0x88

#define NUM_CHANNELS 2  // total number of channels
#define NUM_LINKS 30
#define ADDRESSSTART_WDS_CONF 0x019 // start address of hbw_config_signalduino_wds_7in1 // TODO? make array for multiple channels (start+ sizeof(hbw_config_signalduino_wds_7in1))
#define LINKADDRESSSTART_ACT 0xE6 // start for any ACTUATOR peering (must have same 'address_step')


/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove all debug output. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


#include <FreeRam.h>

// HB Wired protocol and module
#include <HBW_eeprom.h>
#include <HBWired.h>
#include "HBWSIGNALDuino_adv.h"
#include "HBWSIGNALDuino_bresser7in1.h"
#include <HBWLinkInfoEventSensor.h>


/* harware specifics ------------------------ */
#if defined (ARDUINO_ARCH_RP2040)
  EEPROM24* EepromPtr = new EEPROM24(Wire, EEPROM_24LC128);  // 16 kBytes EEPROM @ first I2C / Wire0 interface
#else
  // below customizations are probably not compatible with other architectures
  #error Target Plattform not supported! Please contribute.
#endif

// Pins
// TODO move to own file (pins_default.h pins_custom.h - exclude pins_custom from Git)
#define LED 6      // Signal-LED
#define RS485_TXEN 7  // Transmit-Enable
// UART1==Serial2 for HM bus
#define BUTTON 22  // Button fuer Factory-Reset etc.

// cc1101 module @ SPI0
// #define PIN_SEND              20   // gdo0Pin TX out
// #define PIN_RECEIVE           21   // gdo2
// see: HBWSIGNALDuino_adv/compile_config.h and HBWSIGNALDuino_adv/cc1101.h

// default pins:
// USB Rx / Tx 0, 1 (UART0)
// SPI0[] = {MISO, SS, SCK, MOSI}; // GPIO 16 - 19
// I2C0[] = {PIN_WIRE0_SDA, PIN_WIRE0_SCL}; // GPIO 4 & 5
// UART1 GPIO 8 & 9
// possible to use BOOTSEL button in sketch?

// device config
// different layout/central_address position for RP2040! Variable start must be multiple of four / cannot access a 16bit type at an odd address?
static struct hbw_config {
  uint8_t logging_time;     // 0x01 - eeprom addr
  uint8_t padding[2];     // 0x02 - 03
  uint8_t direct_link_deactivate:1;   // 0x04:0
  uint8_t fillup         :7;   // 0x04:1-7
  uint8_t central_address[4];  // 0x05 - 0x08
  hbw_config_signalduino_adv signalduinoCfg[1]; // 0x09-0x... ? (address step 16)
  hbw_config_signalduino_wds_7in1 wds7in1Cfg[1]; // 0x19-0x... ? (address step 16)
  // TempH channel? (provide temp and humidity in separate channel for peering?)
  // virtual BRIGHTNESS (0...100%) chan?
} hbwconfig;


// create object for the channel
HBWChannel* channels[NUM_CHANNELS];

// new child class, to set custom methods for plattform (RP2040)
class HBWRpiPicoDevice : public HBWDevice {
    public: 
    HBWRpiPicoDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
               Stream* _rs485, uint8_t _txen,
               uint8_t _configSize, void* _config, 
               uint8_t _numChannels, HBWChannel** _channels,
               Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
    HBWDevice(_devicetype, _hardware_version, _firmware_version,
              _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
              _debugstream, linksender, linkreceiver) {
    };
    // virtual void afterReadConfig();

    protected:
      void readConfig() override {         // read config from EEPROM	
        // read EEPROM
        EepromPtr->read(0x01, config, configSize);
        // turn around central address
        uint8_t addr[4];
        for(uint8_t i = 0; i < 4; i++) {
          addr[i] = hbwconfig.central_address[i];
        }
        for(uint8_t i = 0; i < 4; i++) {
          hbwconfig.central_address[i] = addr[3-i];
        }
        // set defaults if values not provided from EEPROM or other device specific stuff
        pendingActions.afterReadConfig = true; // tell main loop to run afterReadConfig() for device and channels
      }
      
      // get central address
      uint32_t getCentralAddress() override {
        return *((uint32_t*)hbwconfig.central_address);
      }
};

// device specific defaults
// void HBWRpiPicoDevice::afterReadConfig() {
//   if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 50;
// };


HBWDevice* device = NULL;


/*--------------------------------------------SIGNALDuino-------------------------------------------------*/
#include "HBWSIGNALDuino_adv/SIGNALDuino.ino.hpp"
/* providing setup() and loop() for core0 */
// external EEPROM started here, too!
// see: HBWSIGNALDuino_adv/compile_config.h and HBWSIGNALDuino_adv/cc1101.h for further config
/*--------------------------------------------SIGNALDuino-------------------------------------------------*/


// core1 running the Homematic device
void setup1()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  // set_sys_clock_khz(48000, true); // Set System clock to 48 MHz
  // TODO not working... Serial USB not working - must be done from core0??
  // set_sys_clock_khz(18000, false);
  // clock_configure(
  //     clk_peri,
  //     0,                                                // No glitchless mux
  //     CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
  //     18000 * 1000,                                     // Input frequency
  //     18000 * 1000                                      // Output (must be same as no divider)
  // );

  delay(1500);  // must wait for core0 to start Wire/EEPROM
 #ifdef DEBUG
  delay(2000);
 #endif
  Serial.begin(115200);  // Serial->USB for debug (shared with SIGNALDuino, turn all debug off?)
  Serial2.begin(19200, SERIAL_8E1);  // RS485 bus @UART1

// EE dump ---------------
// // uint8_t ee_buff[16];
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
//------------
 #ifdef HBW_DEBUG
  Serial.print("Configured F_CPU: "); Serial.println(F_CPU);
  Serial.print("Actual F_CPU:     "); Serial.println(rp2040.f_cpu());
 #endif
  // Serial.println("Reconfigure sys_clk to F_CPU");
  // Serial.flush();

  // set_sys_clock_khz(F_CPU/1000, true);

  // Serial.begin(115200);
  // Serial.print("Actual F_CPU:     "); Serial.println(rp2040.f_cpu());

 #if defined (ARDUINO_ARCH_RP2040)
  // Wire.begin();
  if (! EepromPtr->available())
  {
    Serial.println("No EEPROM!!");
    #if !defined(HBW_DEBUG)
    while (true) {  // stop without EEPROM
      #if defined(Support_WDT)
      RESET_WATCHDOG();
      #endif
      digitalWrite(LED, HIGH);
      delay(200);
      digitalWrite(LED, LOW);
      delay(200);
    }
    #endif
  }
 #endif

  // create channels
  channels[0] = new HBWSIGNALDuino_adv(PIN_RECEIVE, PIN_SEND, PIN_LED, &(hbwconfig.signalduinoCfg[0]));
  channels[1] = new HBWSIGNALDuino_bresser7in1(ccBuf, g_hbw_link, &(hbwconfig.wds7in1Cfg[0]), ADDRESSSTART_WDS_CONF);

  // create the device
  device = new HBWRpiPicoDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                                &Serial2,
                                RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                                NUM_CHANNELS, (HBWChannel**)channels,
  #ifdef HBW_DEBUG
                                &Serial, new HBWLinkInfoEventSensor<NUM_LINKS, LINKADDRESSSTART_ACT>());
  #else
                                NULL, new HBWLinkInfoEventSensor<NUM_LINKS, LINKADDRESSSTART_ACT>());
  #endif

  device->setConfigPins(BUTTON, LED);

 #ifdef HBW_DEBUG
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F(", config size "));hbwdebug(sizeof(hbwconfig));
  hbwdebug(F("\n"));
 #endif

  digitalWrite(LED, LOW);
};


// loop for second core, running HBW device
void loop1() {
  device->loop();
};

// check if HBWLinkInfoEvent support is enabled, when links are set
#if !defined(Support_HBWLink_InfoEvent) && defined(NUM_LINKS)
#error enable/define Support_HBWLink_InfoEvent in HBWired.h!
#endif
