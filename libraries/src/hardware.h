/*
 * hardware.h
 *
 *  Created on: 23.05.2018
 *      Author: loetmeister.de
 */

#ifndef hardware_h
#define hardware_h

//#define _HAS_BOOTLOADER_    // enable bootlader support of the device. BOOTSTART must be defined as well

/* Start Boot Program section and RAM address start */
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
// Boot Size 2048 words
#define BOOTSTART (0x3800)
#endif


#if defined(__AVR_ATmega168__)
#define RAMSTART (0x100)
#define NRWWSTART (0x3800)
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#define RAMSTART (0x100)
#define NRWWSTART (0x7000)
#elif defined (__AVR_ATmega644P__)
#define RAMSTART (0x100)
#define NRWWSTART (0xE000)
#elif defined(__AVR_ATtiny84__)
#define RAMSTART (0x100)
#define NRWWSTART (0x0000)
#elif defined(__AVR_ATmega1280__)
#define RAMSTART (0x200)
#define NRWWSTART (0xE000)
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
#define RAMSTART (0x100)
#define NRWWSTART (0x1800)
#endif


/* sleep macro. Timer for millis() must keep running! Don't sleep too deep... */
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
// "idle" sleep mode (mode 0)
#include <avr/sleep.h>
#define POWERSAVE() set_sleep_mode(0); \
                    sleep_mode();
//#elif defined (__AVR_ATmega644P__)... // TODO: add others
#endif

#endif /* hardware */
