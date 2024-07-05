/*
 * hardware.h
 *
 *  Created on: 23.05.2018
 *      Author: loetmeister.de
 */

#ifndef _hardware_h
#define _hardware_h


#if defined (ARDUINO_ARCH_RP2040)
  // #include <SparkFun_External_EEPROM.h>
  // class ExternalEEPROM;    // forward declare class
  // extern ExternalEEPROM* EepromPtr;
  #include <at24c04.h>
  class AT24C04;
  extern AT24C04* EepromPtr;

  // max EEPROM address for HBW device. Usual EEPROM size is 1024 (as defined in XML)
  #define E2END 0x3FF
  #ifndef NOT_A_PIN
  #define NOT_A_PIN 0xFF
  // #include <Watchdog.h>
  #endif
#else
  #include <EEPROM.h>
  #include "avr/wdt.h"
  extern EEPROMClass* EepromPtr;
#endif

//#define _HAS_BOOTLOADER_    // enable bootlader support of the device. BOOTSTART must be defined as well

/* Start Boot Program section and RAM address start */
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega328PB__)
  // Boot Size 2048 words
  #define BOOTSTART (0x3800)
#elif defined (__AVR_ATmega32__)
  #define BOOTSTART (0x3800)
#endif


#if defined(__AVR_ATmega168__)
  #define RAMSTART (0x100)
  #define NRWWSTART (0x3800)
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
  #define RAMSTART (0x100)
  #define NRWWSTART (0x7000)
#elif defined (__AVR_ATmega644P__)
  #define RAMSTART (0x100)
  #define NRWWSTART (0xE000)
#elif defined(__AVR_ATtiny84__)
  #define RAMSTART (0x100)
  #define NRWWSTART (0x0000)
#elif defined(__AVR_ATmega1280__)
  #define RAMSTART (0x200)
  #define NRWWSTART (0xE000)
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
  #define RAMSTART (0x100)
  #define NRWWSTART (0x1800)
#endif


/* sleep macro. Timer for millis() must keep running! Don't sleep too deep... */
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega328PB__)
  // "idle" sleep mode (mode 0)
  #include <avr/sleep.h>
  #define POWERSAVE() set_sleep_mode(0); \
                      sleep_mode();
#elif defined (ARDUINO_ARCH_RP2040)
  #define POWERSAVE() sleep();
  
//#elif defined (__AVR_ATmega644P__)... // TODO: add others
#endif


/* watchdog macros, config, reboot and reset*/
// #if defined (__AVR__)
  // #include "avr/wdt.h"
  // inline void resetHardware() {
    // while(1){}  // if watchdog is used & active, just run into infinite loop to force reset
  // };
// #elif defined (ARDUINO_ARCH_RP2040)
  // // #include <hardware/watchdog.h>
  // inline void resetHardware() {
  // watchdog_reboot(0, SRAM_END, 0);
  // //watchdog_reboot(0, 0, 10);  // watchdog fire after 10us and busy waits
    // for (;;) {
    // };
// #endif

#endif /* _hardware_h */
