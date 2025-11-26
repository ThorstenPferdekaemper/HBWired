/* Default / example configuation file. When using custom device config (pinout, EEPROM, etc.) or controller,
 * copy this file and adjust it. This would also avoid to owerwrite it with new versions from GitHub. */

// Arduino NANO / ATMega 328p config - RS485 bus is attached to SoftwareSerial by default, UART0 is used for serial debug.

#include <HBW_eeprom.h>
#include <FreeRam.h>
/* harware specifics ------------------------ */
#if defined (ARDUINO_ARCH_RP2040)
  // EEPROM24* EepromPtr = new EEPROM24(Wire, EEPROM_24LC128);  // 16 kBytes EEPROM @ first I2C / Wire0 interface
  EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM
#else
  // below customizations are probably not compatible with other architectures
  #error Target Plattform not supported! Please contribute.
#endif


// Pins
#define LED 6      // Signal-LED
#define RS485_TXEN 7  // Transmit-Enable
// UART1==Serial2 used for HM bus
#define BUTTON 22  // Button fuer Factory-Reset etc.

// cc1101 module @ SPI0
// #define PIN_SEND              20   // gdo0Pin TX out
// #define PIN_RECEIVE           21   // gdo2
// see: HBWSIGNALDuino_adv/compile_config.h and HBWSIGNALDuino_adv/cc1101.h

// default pins:
// USB Rx / Tx 0, 1 (UART0)
// SPI0[] = {MISO, SS, SCK, MOSI}; // GPIO 16 - 19
// I2C0[] = {PIN_WIRE0_SDA, PIN_WIRE0_SCL}; // GPIO 4 & 5
// UART1 (Serial2) GPIOs: 8 & 9
// possible to use BOOTSEL button in sketch?
