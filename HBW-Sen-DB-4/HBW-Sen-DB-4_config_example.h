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
  #define LED LED_BUILTIN        // Signal-LED
  
  #define BUTTON_1 8
  #define BUTTON_2 7
  #define BUTTON_3 4
  #define BUTTON_4 A1

  #define BUZZER 11 // buzzer for key press feedback (timer2)
  #define BACKLIGHT_PWM 9  // timer1 (pin 9 & 10)
  #define LDR_PIN A0
//  #define BLOCKED_TWI_SDA A4  // used by I²C - SDA
//  #define BLOCKED_TWI_SCL A5  // used by I²C - SCL
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define LED LED_BUILTIN        // Signal-LED

  #define BUTTON_1 A0
  #define BUTTON_2 A1
  #define BUTTON_3 A2
  #define BUTTON_4 A3
  
  #define BUZZER 11 // buzzer for key press feedback  (tone() timer prio: timer2, 1, 0)
  // 11, 3 (controlled by timer2), timer0 (pin 5 & 6), timer1 (pin 9 & 10)
  #define BACKLIGHT_PWM 9
  #define LDR_PIN A7
//  #define BLOCKED_TWI_SDA A4  // used by I²C - SDA
//  #define BLOCKED_TWI_SCL A5  // used by I²C - SCL

  #include "FreeRam.h"
  #include "HBWSoftwareSerial.h"
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL


