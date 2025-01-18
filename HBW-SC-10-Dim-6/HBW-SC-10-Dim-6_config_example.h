/* Default / example configuation file. When using custom device config (pinout, EEPROM, etc.) or controller,
 * copy this file and adjust it. This would also avoid to owerwrite it with new versions from GitHub. */

// Arduino NANO / ATMega 328p config - RS485 bus is attached to SoftwareSerial by default, UART0 is used for serial debug.

EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM

// #define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove unneeded code. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */

#include "SetupTimer_328p.h"

inline void SetupHardware()
{
  setupPwmTimer1();
  setupPwmTimer2();
};

// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define LED LED_BUILTIN        // Signal-LED
  
  #define PWM1 9  // PWM out (controlled by timer1)
  #define PWM2_DAC 5  // PWM out (controlled by timer0)
  #define PWM3_DAC 10  // PWM out (controlled by timer1)
  #define PWM4   6 // PWM out (controlled by timer0)
  #define PWM5 11  // PWM out (controlled by timer2)
  #define PWM6 3  // PWM out (controlled by timer2)

  #define IO1 A3
  #define IO2 A2
  #define IO3 A1
  #define IO4 A0
  #define IO5 4
  #define IO6 7
  #define IO7 8
  #define IO8 12
  #define IO9 A4
  #define IO10 A5
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define LED LED_BUILTIN        // Signal-LED

  #define PWM1 9
  #define PWM2_DAC 5
  #define PWM3_DAC 10
  #define PWM4 6
  #define PWM5 11
  #define PWM6 NOT_A_PIN  // dummy pin to fill the array elements

  #define IO1 7
  #define IO2 A5
  #define IO3 12
  #define IO4 A0
  #define IO5 A1
  #define IO6 A2
  #define IO7 A3
  #define IO8 A4
  #define IO9 NOT_A_PIN  // dummy pin to fill the array elements
  #define IO10 NOT_A_PIN  // dummy pin to fill the array elements

  #include "FreeRam.h"
  #include <HBWSoftwareSerial.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL

