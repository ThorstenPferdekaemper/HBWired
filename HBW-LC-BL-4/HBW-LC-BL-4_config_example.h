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
  
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  
  #include <HBWSoftwareSerial.h>
  #include <FreeRam.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif

#define BUTTON 8  // Button fuer Factory-Reset etc.
#define LED LED_BUILTIN        // Signal-LED

#define BLIND1_ACT 5		// "Ein-/Aus-Relais"
#define BLIND1_DIR 6		// "Richungs-Relais"
#define BLIND2_ACT A0
#define BLIND2_DIR A1
#define BLIND3_ACT A2
#define BLIND3_DIR A3
#define BLIND4_ACT A4
#define BLIND4_DIR A5

#define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage (using internal 1.1 volt reference)

inline void SetupHardware()
{
  #if defined (ADC_BUS_VOLTAGE)
  analogReference(INTERNAL);    // select internal 1.1 volt reference (to measure external bus voltage)
  #endif
};

