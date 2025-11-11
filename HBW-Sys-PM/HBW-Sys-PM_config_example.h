/* Default / example configuation file. When using custom device config (pinout, EEPROM, etc.) or controller,
 * copy this file and adjust it. This would also avoid to owerwrite it with new versions from GitHub. */

// Arduino NANO / ATMega 328p config - RS485 bus is attached to SoftwareSerial by default, UART0 is used for serial debug.

#include <HBW_eeprom.h>
EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM

// #define USE_HARDWARE_SERIAL   // use hardware serial (USART) for final device - this disables debug output
/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove unneeded code. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */

// #if defined (ARDUINO_ARCH_RP2040)
// #define LED 6      // Signal-LED
// #define RS485_TXEN 7  // Transmit-Enable
// // UART1==Serial2 for HM bus
// #define BUTTON 22  // Button fuer Factory-Reset etc.
// #endif


//  #define TWI_SDA SDA  // A4  used for I²C
//  #define TWI_SCL SCL  // A5  used for I²C
SBCDVA INA236(0x40, &Wire);  // first "Wire" interface is the default
// Supply voltage 1,7 - 5,5 V 
// Current measuring range 0 - 8 A 
// Logic level 1,7 - 5,5 V 
// "Common-mode" Voltage range 0 - 48 V DC

// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  #define BUTTON A6  // Button fuer Factory-Reset etc.
  #define LED LED_BUILTIN      // Signal-LED
  
  #define SWITCH1_PIN 7   // Ausgangpins fuer open collector 1
  #define SWITCH2_PIN 8   // Ausgangpins fuer open collector 2
  #define SWITCH3_PIN A1  // Ausgangpins fuer Relais 1
  #define SWITCH4_PIN 4   // Ausgangpins fuer Relais 2

  #define INPUT1_PIN A2  // Input 1
  #define INPUT2_PIN A3  // Input 2
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  #define BUTTON 8  // Button fuer Factory-Reset etc.
  #define LED LED_BUILTIN      // Signal-LED

  #define SWITCH1_PIN 6  // Ausgangpins fuer die Relais
  #define SWITCH2_PIN 7
  #define SWITCH3_PIN A1
  #define SWITCH4_PIN 10
  
  #define INPUT1_PIN A2  // Input 1
  #define INPUT2_PIN A3  // Input 2

  #include <HBWSoftwareSerial.h>
  #include <FreeRam.h>  
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif

