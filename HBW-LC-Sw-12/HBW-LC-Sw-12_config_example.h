/* Default / example configuation file. When using custom device config (pinout, EEPROM, etc.) or controller,
 * copy this file and adjust it. This would also avoid to owerwrite it with new versions from GitHub. */

// Arduino NANO / ATMega 328p config - RS485 bus is attached to SoftwareSerial by default, UART0 is used for serial debug.

EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM

// #define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove unneeded code. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */


// Pins
#ifdef USE_HARDWARE_SERIAL
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define RS485_TXEN 2  // Transmit-Enable
  #define shiftReg_OutputEnable 8   // OE output enable, connect to all shift register
  // 6 realys and LED attached to 3 shiftregisters
  #define shiftRegOne_Data  10       //DS serial data input
  #define shiftRegOne_Clock 3       //SH_CP shift register clock input
  #define shiftRegOne_Latch 4       //ST_CP storage register clock input
  // extension shifregister for another 6 relays and LEDs
  #define shiftRegTwo_Data  7
  #define shiftRegTwo_Clock 12
  #define shiftRegTwo_Latch 9
  
#else
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define shiftReg_OutputEnable 12   // OE output enable, connect to all shift register
  // 6 realys and LED attached to 3 shiftregisters
  #define shiftRegOne_Data  10       //DS serial data input
  #define shiftRegOne_Clock 9       //SH_CP shift register clock input
  #define shiftRegOne_Latch 11       //ST_CP storage register clock input
  // extension shifregister for another 6 relays and LEDs
  #define shiftRegTwo_Data  5
  #define shiftRegTwo_Clock 7
  #define shiftRegTwo_Latch 6
  
  #include <FreeRam.h>
  #include <HBWSoftwareSerial.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif

#define LED LED_BUILTIN        // Signal-LED

#define CT_PIN1 A0  // analog input for current transformer, switch channel 1
#define CT_PIN2 A1  // analog input for current transformer, switch channel 2
#define CT_PIN3 A2
#define CT_PIN4 A4
#define CT_PIN5 A5
#define CT_PIN6 A3


