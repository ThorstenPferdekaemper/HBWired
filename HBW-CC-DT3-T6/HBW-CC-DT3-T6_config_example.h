/* Default / example configuation file. When using custom device config (pinout, EEPROM, etc.) or controller,
 * copy this file and adjust it. This would also avoid to owerwrite it with new versions from GitHub. */

// Arduino NANO / ATMega 328p config - RS485 bus is attached to SoftwareSerial by default, UART0 is used for serial debug.

EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM

// #define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove unneeded code. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */

#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage (optional)

  #define ONEWIRE_PIN	A3 // Onewire Bus
  #define RELAY_1 5
  #define RELAY_2 6
  #define RELAY_3 3

  #define LED LED_BUILTIN        // Signal-LED
  // #define BLOCKED_TWI_SDA A4  // used by I²C - SDA
  // #define BLOCKED_TWI_SCL A5  // used by I²C - SCL

#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage (optional)

  #define ONEWIRE_PIN	10 // Onewire Bus
  #define RELAY_1 5
  #define RELAY_2 6
  #define RELAY_3 A2

  #define LED LED_BUILTIN        // Signal-LED

  #include <FreeRam.h>
  #include <HBWSoftwareSerial.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX

#endif

