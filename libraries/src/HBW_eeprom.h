/*
 * HBW_eeprom.h
 * Helper to allow for different EEPROM libs / different boards
 *
 *  Created on: 10.07.2024
 *      Author: loetmeister.de
 */

#ifndef _HBW_eeprom_h
#define _HBW_eeprom_h


#if defined (ARDUINO_ARCH_RP2040)
  #include <at24c128.h>
  class AT24C128;    // forward declare class
  extern AT24C128* EepromPtr;
  // #include <Eeprom24C04_16.h>
  // class Eeprom24C04_16;    // forward declare class
  // extern Eeprom24C04_16* EepromPtr;

  // max EEPROM address for HBW device. Usual EEPROM size is 1024 (as defined in device XML)
  #define E2END 0x3FF
#else
  // define this, if the class has no update() function, or you don't want to use it
  #define EEPROM_no_update_function
  #include <EEPROM.h>
  extern EEPROMClass* EepromPtr;
#endif


#endif /* _HBW_eeprom_h */
