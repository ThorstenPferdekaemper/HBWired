/*
 * HBW_eeprom.h
 *
 * Helper to allow for different EEPROM libs / different boards
 *
 * The EEPROM class must have write() and read() methods for single bytes.
 * When it does only write after read, you may want to use update() instead.
 * For I2C EEPROMs, it should also have an available() method.
 *
 *  Created on: 10.07.2024
 *      Author: loetmeister.de
 */

#ifndef _HBW_eeprom_h
#define _HBW_eeprom_h


#if defined (ARDUINO_ARCH_RP2040)
  #include <EEPROM24.h>
  // class EEPROM24;    // forward declare class
  // extern EEPROM24* EepromPtr;
  // define this, if the class has no update() function, or you don't want to use it
  // #define EEPROM_no_update_function
  #include <EEPROM.h>
  extern EEPROMClass* EepromPtr;
  #define EEPROM_RPI_simulated
  
  // max EEPROM address for HBW device. Usual EEPROM size is 1024 (as defined in device XML)
  #define E2END 0x3FF
#else
  // define this, if the class has no update() function, or you don't want to use it
  #define EEPROM_no_update_function
  #include <EEPROM.h>
  extern EEPROMClass* EepromPtr;
#endif


#endif /* _HBW_eeprom_h */
