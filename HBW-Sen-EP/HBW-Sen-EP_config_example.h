/* Default / example configuation file. When using custom device config (pinout, EEPROM, etc.) or controller,
 * copy this file and adjust it. This would also avoid to owerwrite it with new versions from GitHub. */

// Arduino NANO / ATMega 328p config - RS485 bus is attached to SoftwareSerial by default, UART0 is used for serial debug.

EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM

// #define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove unneeded code. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define LED LED_BUILTIN        // Signal-LED

  #define Sen1 A0    // digital input
  #define Sen2 A1
  #define Sen3 A2
  #define Sen4 9
  #define Sen5 10
  #define Sen6 11
  #define Sen7 4
  #define Sen8 7

//  #define BLOCKED_TWI_SDA SDA //A4  // reserved for I²C
//  #define BLOCKED_TWI_SCL SCL //A5  // reserved for I²C

#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define LED LED_BUILTIN        // Signal-LED

  #define Sen1 A0    // digital input
  #define Sen2 A1
  #define Sen3 A2
  #define Sen4 9//A3
  #define Sen5 10//A4
  #define Sen6 11//A5
  #define Sen7 5
  #define Sen8 7
  
  #include "FreeRam.h"
  #include <HBWSoftwareSerial.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL


