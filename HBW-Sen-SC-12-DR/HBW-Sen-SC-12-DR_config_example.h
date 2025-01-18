/* Default / example configuation file. When using custom device config (pinout, EEPROM, etc.) or controller,
 * copy this file and adjust it. This would also avoid to owerwrite it with new versions from GitHub. */

// Arduino NANO / ATMega 328p config - RS485 bus is attached to SoftwareSerial by default, UART0 is used for serial debug.

EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM

// #define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove unneeded code. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */

// Pins
  #define IO1 A3
  #define IO2 10
  #define IO3 11
  #define IO4 A0
  #define IO5 A1
  #define IO6 A2
  #define IO7 A4
  #define IO8 A5
  #define IO9 9
  #define IO10 7
  #define IO11 6
  #define IO12 5
  
  #define LED LED_BUILTIN        // Signal-LED

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


