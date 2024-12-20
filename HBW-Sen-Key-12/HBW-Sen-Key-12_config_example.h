/* Default / example configuation file. When using custom device config (pinout, EEPROM, etc.) or controller,
 * copy this file and adjust it. This would also avoid to owerwrite it with new versions from GitHub. */

// Arduino NANO / ATMega 328p config - RS485 bus is attached to SoftwareSerial by default, UART0 is used for serial debug.

EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM

// #define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove unneeded code. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */

// Pins
#define RS485_RXD 4
#define RS485_TXD 2
#define RS485_TXEN 3  // Transmit-Enable

#include <FreeRam.h>
#include <HBWSoftwareSerial.h>
HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX

#define BUTTON 8  // Button fuer Factory-Reset etc.
#define LED LED_BUILTIN     // Signal-LED

#define Key1 A0
#define Key2 A1
#define Key3 A2
#define Key4 A3
#define Key5 A4
#define Key6 A5
#define Key7 5
#define Key8 6
#define Key9 7
#define Key10 9
#define Key11 10
#define Key12 11

