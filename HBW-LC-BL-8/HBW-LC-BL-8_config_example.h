/* Default / example configuation file. When using custom device config (pinout, EEPROM, etc.) or controller,
 * copy this file and adjust it. This would also avoid to owerwrite it with new versions from GitHub. */

// Arduino NANO / ATMega 328p config - RS485 bus is attached to UART0. No serial debug available.

EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM

/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove unneeded code. "HBW_DEBUG" also works as master switch,
 * as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */

// Pins
#define RS485_TXEN 2  // Transmit-Enable

#define BUTTON A6  // Button fuer Factory-Reset etc.
#define LED LED_BUILTIN        // Signal-LED

#define BLIND1_ACT 10		// "Ein-/Aus-Relais"
#define BLIND1_DIR 9		// "Richungs-Relais"
#define BLIND2_ACT 8
#define BLIND2_DIR 7
#define BLIND3_ACT 6
#define BLIND3_DIR 5
#define BLIND4_ACT 4
#define BLIND4_DIR 3

#define BLIND5_ACT A5
#define BLIND5_DIR A4
#define BLIND6_ACT A2
#define BLIND6_DIR A3
#define BLIND7_ACT A1
#define BLIND7_DIR A0
#define BLIND8_ACT 11
#define BLIND8_DIR 12

#define ADC_BUS_VOLTAGE A7  // analog input to measure bus voltage (using internal 1.1 volt reference)


